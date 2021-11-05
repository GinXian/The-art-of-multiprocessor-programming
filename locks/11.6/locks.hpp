#ifndef __LOCKS_HEADER__
#define __LOCKS_HEADER__

#include <atomic>
#include "lock_traits.hpp"

template <class Mutex = CompositeLock,
        typename = std::enable_if_t<std::is_same_v<typename lock_traits<Mutex>::is_lock_type, std::true_type>, void>>
class lock_guard {
private:
    Mutex& m;
public:
    lock_guard() = delete;
    explicit lock_guard(Mutex& other) : m(other) { m.lock(); }
    ~lock_guard() { m.unlock(); }
};

// If it is not a lock type, do nothing.
template <class notMutex,
        typename = std::enable_if_t<std::is_same_v<typename lock_traits<Mutex>::is_lock_type, std::false_type>, void>
class lock_guard {};

/* Partial Specialization will do the same as SFINAE does */
/* template <class Mutex, bool = std::is_same_v<..., std::true_type>> */

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
        while (!std::atomic_compare_exchange_weak_explicit(&lock, true, false)) {}
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
            if(std::atomic_compare_exchange_weak_explicit(&tail, true, false))
                return;
        }
    }
    void unlock() {
        lock = true;
    }
};

class ALock {
private:
    std::atomic<int> tail{};
    bool *queue;
    static thread_local int mySlotIndex;
    int capacity;
    friend class lock_guard<ALock, void>;

public:
    explicit ALock(int capacity = 10) : capacity(capacity) {
        tail = 0;
        queue = new bool[capacity]{false};
        mySlotIndex = -1;
    }
    ~ALock() { delete[] queue; }

private:
    void lock() {
        mySlotIndex = std::atomic_fetch_add(&tail, 1) % capacity;
        while (!queue[mySlotIndex]) {}
        queue[mySlotIndex] = false;
    }
    void unlock() {
        queue[(mySlotIndex + 1) % capacity] = true;
    }
};

class CLHLock {
private:
    std::atomic<bool*> tail{};
    static thread_local bool *myPred;
    static thread_local bool *myNode;
    friend class lock_guard<CLHLock, void>;

public:
    CLHLock() {
        tail = new bool{true};
        myPred = myNode = nullptr;
    }
    ~CLHLock() {
        if (nullptr != myNode)
            delete myNode;
        if (nullptr != myPred)
            delete myPred;
    }

private:
    void lock() {
        if (nullptr == myNode)
            myNode = new bool{false};
        myPred = std::atomic_exchange(&tail, myNode);
        while (!*myPred) {}
        *myPred = false;
    }
    void unlock() {
        *myNode = true;
        myNode = myPred;
        myPred = nullptr;
    }
};

class MCSLock {
private:
    typedef struct node {
        bool lock;
        struct node *next;
    } Node;
    static thread_local Node *myNode;
    std::atomic<Node*> tail{};
    friend class lock_guard<MCSLock, void>;

public:
    MCSLock() {
        tail = myNode = nullptr;
    }
    ~MCSLock() {
        if (nullptr != myNode)
            delete myNode;
    }

private:
    void lock() {
        if (nullptr == myNode) {
            myNode = new Node{false, nullptr};
        }
        Node *pred = std::atomic_exchange(&tail, myNode);
        if (nullptr != pred) {
            // The lock is occupied now.
            // So you have to wait for the predecessor to pass you the lock.
            pred->next = myNode;
            while (!myNode->lock) {}
            myNode->lock = false;

        } else {
            // The lock is not occupied, so directly get it from the boss.
        }

    }
    void unlock() {
        if (nullptr == myNode->next) {  // If there is no new-comers after you
            if (std::atomic_compare_exchange_weak_explicit(&tail, myNode, nullptr))
                // At second glance, still no new-comer. Return the lock to the boss.
                return;
            // At second glance, a new-comer has come.
            while (nullptr == myNode->next) {
                /* Wait for new-comer to tell you his location so that you can pass the lock */
                /* Risk of blocking if the new-comer died before telling you his location. */
                /* TODO: add a counter or timer here to avoid starvation. */
            }
        }
        // Pass the lock
        myNode->next->lock = true;
        myNode->next = nullptr;
    }
};

#endif