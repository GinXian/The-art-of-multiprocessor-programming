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
        while (std::atomic_compare_exchange_weak_explicit(&tail, true, false)) {}
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
            if (std::atomic_compare_exchange_weak_explicit(&tail, true, false))
                return;
        }
    }
};

class ALock {
private:
    int capacity;
    bool *queue;
    std::atomic<int> tail{};
    static thread_local int mySlotIndex;
    friend class lock_guard<ALock, void>;

public:
    ALock(ALock& other) = delete;
    ALock& operator=(ALock&) = delete;
    explicit ALock(int capacity = 10) : capacity(capacity) {
        queue = new bool[capacity]{false};
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
        mySlotIndex = std::atomic_fetch_add(&tail, 1) % capacity;
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
    std::atomic<bool*> tail{};
    static thread_local bool *myPred;
    static thread_local std::unique_ptr<bool> myNode{nullptr};
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
        myPred = std::atomic_exchange(&tail, myNode.get());
        while (!*myPred) {}
        *myPred = false;
    }
    void unlock() {
        *myNode = true;
        myNode.reset(myNode);
        myNode = nullptr;
    }
};

class MCSLock {
private:
    typedef struct node {
        bool lock;
        struct node *next;
    } Node;
    std::atomic<Node*> tail{};
    static thread_local Node myNode{};
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
        Node *pred = std::atomic_exchange(&tail, &myNode);
        if (nullptr != pred) {
            pred->next = &myNode;
            while (!myNode.lock) {}
            myNode.lock = false;
        }
    }
    void unlock() {
        if (nullptr == myNode.next) {
            if (std::atomic_compare_exchange_weak_explicit(&tail, &myNode, nullptr))
                return;
            while (nullptr == myNode.next) {}
        }
        myNode.next->lock = true;
        myNode.next = nullptr;
    }
};


#endif