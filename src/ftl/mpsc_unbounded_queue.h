#pragma once

#include <atomic>
#include <memory>
#include <optional>

namespace ftl {

struct AtomicNextNodeBase {
    std::atomic<AtomicNextNodeBase*> pNext = nullptr;
};

// unbounded lockfree generic MPSCQueue
class MPSCUnboundedNodeQueue {

    AtomicNextNodeBase mStub; // tail
    std::atomic<AtomicNextNodeBase*> mHead; // push to head, pop from tail
    AtomicNextNodeBase* mTail = nullptr;

    std::atomic<size_t> mSize = 0;

    MPSCUnboundedNodeQueue(const MPSCUnboundedNodeQueue&) = delete;
    MPSCUnboundedNodeQueue& operator=(const MPSCUnboundedNodeQueue&) = delete;

public:
    MPSCUnboundedNodeQueue()
        : mHead(&mStub)
        , mTail(&mStub)
    {
    }
    ~MPSCUnboundedNodeQueue()
    { // allow destructing non-empty queue
    }

    void push(AtomicNextNodeBase* pNode)
    {
        pNode->pNext.store(nullptr, std::memory_order_relaxed);
        auto phead = mHead.exchange(pNode, std::memory_order_acq_rel);
        phead->pNext.store(pNode, std::memory_order_release);
    }

    size_t size() const
    {
        return mSize.load();
    }
    bool empty() const
    {
        return size() == 0;
    }

    AtomicNextNodeBase* top()
    {
        if (mTail == &mStub) // possibly empty
        {
            if (auto pnext = mTail->pNext.load(std::memory_order_acquire))
                mTail = pnext; // update tail
            else
                return nullptr; // empty
        }
        return mTail;
    }
    AtomicNextNodeBase* pop()
    {
        if (mTail == &mStub) // possibly empty
        {
            if (auto pnext = mTail->pNext.load(std::memory_order_acquire))
                mTail = pnext; // update tail
            else
                return nullptr; // empty
        }
        auto tail = mTail;
        if (!mTail->pNext.load(std::memory_order_acquire)) // only 1 elements
            push(&mStub); // push mStub, so that mHead is updated
        mTail = tail->pNext.load(std::memory_order_acquire); // now mTail may or may not point to mStub
        mSize.fetch_sub(1);
        return tail;
    }
};

// unbounded lockfree MPSCQueue
template <class T, class Alloc = std::allocator<T>>
class MPSCUnboundedQueue {

    struct Node : AtomicNextNodeBase {
        T val;
    };
    using allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;

    MPSCUnboundedNodeQueue mQue;
    Alloc mAlloc;

    MPSCUnboundedQueue(const MPSCUnboundedQueue&) = delete;
    MPSCUnboundedQueue& operator=(const MPSCUnboundedQueue&) = delete;

public:
    MPSCUnboundedQueue(Alloc&& alloc = Alloc())
        : mQue()
        , mAlloc(alloc)
    {
    }
    ~MPSCUnboundedQueue()
    {
        while (pop())
            ;
    }

    template <class... Args>
    void push(Args&&... args)
    {
        auto pNode = mAlloc.allocate(1);
        new (&pNode->val) T(std::forward<Args>(args)...);
        mQue.push(pNode);
    }

    size_t size() const
    {
        return mQue.size();
    }
    bool empty() const
    {
        return mQue.empty();
    }

    T* top()
    {
        if (auto pNode = static_cast<Node*>(mQue.top())) {
            return &pNode->val;
        }
        return nullptr;
    }
    void pop()
    {
        if (auto pNode = static_cast<Node*>(mQue.pop())) {
            pNode->val.~T();
            mAlloc.deallocate(pNode);
        }
    }
};
}
