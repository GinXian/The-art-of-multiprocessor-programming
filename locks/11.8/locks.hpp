#ifndef __LOCKS_HEADER__
#define __LOCKS_HEADER__

#include "lock_traits.hpp"
#include <atomic>
#include <memory>

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
        typename = std::enable_if_t<std::is_same_v<lock_traits<Mutex>::is_lock_type, std::false_type>, void>>
class lock_guard {};

class TASLock {
private:
    std::atomic<bool> lock{};
    friend class lock_guard<TASLock, void>;

public:
    TASLock() {
        lock = true;
    }

private:
    void lock() {
        while (std::atomic_compare_exchange_weak_explicit(&tail, true, false, std::memory_order_relaxed)) {}
    }
    void unlock() {
        lock = true;
    }
};

class TTASLock {
private:
    std::atomic<bool> lock{};
    friend class lock_guard<TTASLock, void>;

public:
    TTASLock() {
        lock = true;
    }

private:
    void lock() {
        while (true) {
            while (!lock) {}
            if (std::atomic_compare_exchange_weak_explicit(&lock, true, false, std::memory_order_relaxed))
                return;
        }
    }
};

class ALock {
private:
    int capacity;
    volatile bool *queue;
    std::atomic<int> tail{};
    static thread_local int mySlotIndex;
    friend class lock_guard<ALock, void>;

public:
    ALock(ALock& other) = delete;
    ALock& operator=(ALock&) = delete;
    explicit ALock(int capacity = 10) : capacity(capacity) {
        queue = new volatile bool[capacity]{false};
        tail = 0;
        queue[0] = true;
        mySlotIndex = -1;
    }
    ~ALock() {
        if (queue != nullptr)
            delete[] queue;
    }

private:
    void lock() {
        mySlotIndex = std::atomic_fetch_add_explicit(&tail, 1, std::memory_order_relaxed) % capacity;
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
    std::atomic<volatile bool*> tail{};
    static thread_local volatile bool *myPred;
    static thread_local std::unique_ptr<volatile bool> myNode{nullptr};
    friend class lock_guard<CLHLock, void>;

public:
    CLHLock() {
        tail = new bool{true};
        myPred = nullptr;
    }
    ~CLHLock() {
        if (tail != nullptr)
            delete tail;
    }

private:
    void lock() {
        if (nullptr == myNode.get())
            myNode.reset(new bool{false});
        myPred = std::atomic_exchange_explicit(&tail, myNode.get(), std::memory_order_relaxed);
        while (!*myPred) {}
        *myPred = false;
    }
    void unlock() {
        *myNode = true;
        myNode.reset(myPred);
        myPred = nullptr;
    }
};

class MCSLock {
private:
    typedef struct node {
        bool lock;
        struct node *next;
    } Node;
    std::atomic<volatile Node*> tail{};
    static thread_local volatile Node myNode{};
    friend class lock_guard<MCSLock, void>;

public:
    MCSLock() {
        tail = nullptr;
        myNode.lock = false;
        myNode.next = nullptr;
    }
    ~MCSLock() {
        if (tail != nullptr)
            delete tail;
    }

private:
    void lock() {
        Node *pred = std::atomic_exchange_explicit(&tail, &myNode, std::memory_order_relaxed);
        if (nullptr != pred) {
            pred->next = &myNode;
            while (!myNode.lock) {}
            myNode.lock = false;
        }
    }
    void unlock() {
        if (nullptr == myNode.next) {
            Node *tmp = &myNode;
            // atomic_compare_exchange will update the expected value if the load failed.
            if (std::atomic_compare_exchange_weak_explicit(&tail, &tmp, nullptr, std::memory_order_relaxed))
                return;
            while (nullptr == myNode.next) {}
        }
        myNode.next->lock = true;
        myNode.next = nullptr;
    }
};


#endif