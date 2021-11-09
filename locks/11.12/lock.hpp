#ifndef __LOCKS_HEADER__
#define __LOCKS_HEADER__

#include "lock_traits.hpp"
#include <atomic>
#include <memory>

namespace multicore {
    template <class Mutex,
            typename = std::enable_if_t<std::is_same_v<lock_traits<Mutex>::is_lock_type, std::true_type>, void>>
    class lock_guard {
        Mutex& m;
    public:
        lock_guard() = delete;
        lock_guard(Mutex& other) : m(other) { m.lock(); }
        ~lock_guard() { m.unlock(); }
    };

    template <class Mutex,
            typename = std::enable_if_t<std::is_same_v<lock_traits<Mutex>::is_lock_type, std::false_type>, void*>>
    class lock_guard {};

    class TASLock {
    private:
        std::atomic<volatile bool> lock{true};
        friend class lock_guard<TASLock, void>;

    public:
        TASLock() = default;
        ~TASLock() = default;

    private:
        void lock() {
            bool expected = true;
            while (!atomic_compare_exchange_weak_explicit(expected, false, std::memory_order_relaxed)) {
                expected = true;
            }
        }
        void unlock() {
            lock.store(true, std::memory_order_relaxed);
        }
    };

    class TTASLock {
    private:
        std::atomic<volatile bool> lock{true};
        friend class lock_guard<TTASLock, void>;

    private:
        void lock() {
            bool expected;
            while (true) {
                while (!lock.load(std::memory_order_acquire)) {}
                expected = true;    // Need protecting by release-order
                if (lock.compare_exchange_weak_explicit(expected, false, std::memory_order_release))
                    return;
            }
        }
        void unlock() {
            lock.store(true, std::memory_order_relaxed);
        }
    };

    class ALock {
    private:
        std::atomic<int> tail{0};
        volatile bool *queue;
        int capacity;
        static thread_local int mySlotIndex = -1;
        friend class lock_guard<ALock, void>;

    public:
        ALock(int capacity = 16) : capacity(capacity) {
            queue = new volatile bool[capacity]{false};
            queue[0] = true;
        }
        ~ALock() {
            if (queue != nullptr)
                delete[] queue;
        }

    private:
        void lock() {
            mySlotIndex = std::atomic_fetch_add_explicit(&tail, 1, std::memory_order_consume) % capacity;
            while (!queue[mySlotIndex]) {}
            queue[mySlotIndex] = false;
        }
        void unlock() {
            if (-1 == mySlotIndex)
                return;
            queue[(mySlotIndex + 1) % capacity] = true;
            mySlotIndex = -1;
        }
    };

    class CLHLock {
    private:
        std::atomic<volatile bool*> tail{nullptr};
        static thread_local std::shared_ptr<volatile bool*> myNode{nullptr};
        static thread_local std::shared_ptr<volatile bool*> myPred{nullptr};
        friend class lock_guard<CLHLock, void>;

    public:
        CLHLock() {
            tail = new volatile bool{true};
        }
        ~CLHLock() {
            if (tail != nullptr)
                delete tail;
        }

    private:
        void lock() {
            if (nullptr == myNode.get())
                myNode.reset(new volatile bool{false});
            myPred.reset(tail.exchange(myNode.get(), std::memory_order_consume));
            while (!(*myPred)) {}
            *myPred = false;
        }
        void unlock() {
            *myNode = true;
            myNode = std::move(myPred);
        }
    };

    class MCSLock {
    private:
        typedef struct node {
            bool lock;
            struct node *next;
        } Node;
        std::atomic<volatile Node*> tail{nullptr};
        static thread_local volatile Node myNode{false, nullptr};
        friend class lock_guard<MCSLock, void>;

    public:
        MCSLock() = default;
        ~MCSLock() {
            if (tail != nullptr)
                delete tail;
        }

    private:
        void lock() {
            volatile Node *pred = tail.exchange(&myNode, std::memory_order_consume);
            if (pred != nullptr) {
                pred->next = &myNode;
                while (!myNode.lock) {}
                myNode.lock = false;
            }
        }
        void unlock() {
            if (nullptr == myNode.next) {
                Node *expected = &myNode;
                if (tail.compare_exchange_weak(expected, nullptr, std::memory_order_relaxed))
                    return;
                while (nullptr == myNode.next) {}
            }
            myNode.next->lock = true;
            myNode.next = nullptr;
        }
    };
}


#endif