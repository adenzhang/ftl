#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <optional>

namespace ftl {

// MPSC lock free bounded array queue
template <class T, class Alloc = std::allocator<T>, size_t Align = 64>
class MPSCBoundedQueue {
public:
    using size_type = ptrdiff_t; // signed
    using seq_type = std::atomic<size_type>;
    struct Entry {
        typename std::aligned_storage<sizeof(seq_type), Align>::type mSeq;
        typename std::aligned_storage<sizeof(T), Align>::type mData;

        seq_type& seq()
        {
            return *reinterpret_cast<seq_type*>(&mSeq);
        }
        T& data()
        {
            return *reinterpret_cast<T*>(&mData);
        }
    };
    using allocator_type = Alloc;
    using node_allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<Entry>;

protected:
    node_allocator mAlloc;
    size_t mCap = 0;
    Entry* mBuf = nullptr;

    seq_type mPushPos = 0, mPopPos = 0;

public:
    using value_type = T;

    MPSCBoundedQueue(size_t cap, allocator_type&& alloc = allocator_type())
        : mAlloc(alloc)
        , mCap(cap)
    {
        mBuf = mAlloc.allocate(cap);
        if (!mBuf)
            return;
        for (size_t i = 0; i < cap; ++i) {
            new (&mBuf[i].seq()) seq_type();
            mBuf[i].seq() = 0;
        }
    }
    ~MPSCBoundedQueue()
    {

        if (mBuf) {
            while (top())
                pop();
            mAlloc.deallocate(mBuf, mCap);
            mBuf = nullptr;
        }
    }
    MPSCBoundedQueue(const MPSCBoundedQueue&) = delete;
    MPSCBoundedQueue& operator=(const MPSCBoundedQueue&) = delete;

    template <class... Args>
    bool push(Args... args)
    {
        for (;;) {
            auto pushpos = mPushPos.load(std::memory_order_relaxed);
            auto& entry = mBuf[pushpos % mCap];
            auto seq = entry.seq().load(std::memory_order_acquire);
            size_type diff = seq - pushpos;

            if (diff == 0) { // acquired pushpos & seq
                if (mPushPos.compare_exchange_weak(pushpos, pushpos + 1, std::memory_order_relaxed)) {
                    new (&entry.data()) T(std::forward<Args>(args)...);
                    entry.seq().store(seq + 1, std::memory_order_release); // inc seq. when popping, seq == poppos+1
                    return true;
                } // else pushpos was acquired by other push thread, retry
            } else if (diff < 0) { // full
                return false;
            } // else pushpos was acquired by other push thread, retry
        }
    }

    // buf: uninitialized memory
    bool pop(T* buf = nullptr)
    {
        auto poppos = mPopPos.load(std::memory_order_acquire);
        auto& entry = mBuf[poppos % mCap];
        auto seq = entry.seq().load(std::memory_order_acquire);
        auto diff = seq - poppos - 1;

        if (diff == 0) {
            mPopPos.store(poppos + 1, std::memory_order_relaxed);
            if (buf)
                new (buf) T(std::move(entry.data()));
            entry.data().~T();
            entry.seq().store(seq - 1 + mCap); // dec seq and plus mCap. when next push, seq == pushpos.
            return true;
        } else { // empty
            assert(diff < 0);
            return false;
        }
    }
    T* top()
    {
        auto poppos = mPopPos.load(std::memory_order_acquire);
        auto& entry = mBuf[poppos % mCap];
        auto seq = entry.seq().load(std::memory_order_acquire);
        auto diff = seq - poppos - 1;

        if (diff == 0) {
            return &entry.data();
        } else { // empty
            assert(diff < 0);
            return nullptr;
        }
    }

    std::size_t size() const
    {
        auto poppos = mPopPos.load(std::memory_order_relaxed);
        auto pushpos = mPushPos.load(std::memory_order_relaxed);
        if (pushpos < poppos)
            return 0;
        return pushpos - poppos;
    }
    bool empty() const
    {
        return size() == 0;
    }
    bool full() const
    {
        return size() == mCap;
    }
};
}
