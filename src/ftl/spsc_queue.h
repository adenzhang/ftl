#pragma once

#include <atomic>
#include <memory>

namespace ftl {

// lockfree SPSCRingQueue
template <class T, class Delete = std::default_delete<T>>
class SPSCRingQueue {
    Delete mDel;
    const size_t mCap = 0;
    T* mBuf = nullptr;
    std::atomic<size_t> mPushPos = 0, mPopPos = 0;

    SPSCRingQueue(const SPSCRingQueue&) = delete;
    SPSCRingQueue& operator=(const SPSCRingQueue&) = delete;

public:
    SPSCRingQueue(size_t cap, T* buf, Delete&& del = Delete())
        : mDel(del)
        , mCap(cap)
        , mBuf(buf)
    {
    }
    SPSCRingQueue(SPSCRingQueue&& a)
        : mDel(a.mDel)
        , mCap(a.mCap)
        , mBuf(a.mBuf)
        , mPushPos(a.mPushPos)
        , mPopPos(a.mPopPos)
    {
        a.mBuf = nullptr;
        a.mPushPos = 0;
        a.mPopPos = 0;
    }
    ~SPSCRingQueue()
    {
        destruct();
    }
    void destruct()
    {
        if (mBuf) {
            mDel(mBuf, mCap);
            mBuf = nullptr;
        }
    }

    size_t capacity() const
    {
        return mCap - 1;
    }
    bool full() const
    {
        return mCap == 0 || (mPushPos.load(std::memory_order_relaxed) + 1) % mCap == mPopPos.load(std::memory_order_relaxed);
    }
    bool empty() const
    {
        return mCap == 0 || mPushPos.load(std::memory_order_relaxed) == mPopPos.load(std::memory_order_relaxed);
    }
    size_t size() const
    {
        auto pushpos = mPushPos.load(std::memory_order_relaxed) % mCap;
        auto poppos = mPopPos.load(std::memory_order_relaxed) % mCap;
        return pushpos > poppos ? (pushpos - poppos) : (poppos - pushpos - 1);
    }

    template <class... Args>
    T* push(Args&&... args)
    {
        if (!mCap)
            return nullptr;
        auto pushpos = mPushPos.load(std::memory_order_relaxed) % mCap;
        auto poppos = mPopPos.load(std::memory_order_acquire) % mCap;
        if ((pushpos + 1) % mCap == poppos)
            return nullptr;
        new (mBuf + pushpos) T(std::forward<Args>(args)...);
        mPushPos.fetch_add(1, std::memory_order_acq_rel);
        return mBuf + pushpos;
    }

    T* top()
    {
        if (!mCap)
            return nullptr;
        auto pushpos = mPushPos.load(std::memory_order_acquire) % mCap;
        auto poppos = mPopPos.load(std::memory_order_relaxed) % mCap;
        if (pushpos == poppos)
            return nullptr;
        return &mBuf[poppos];
    }
    T* pop()
    {
        if (!mCap)
            return nullptr;
        auto pushpos = mPushPos.load(std::memory_order_acquire) % mCap;
        auto poppos = mPopPos.load(std::memory_order_relaxed) % mCap;
        if (pushpos == poppos)
            return nullptr;
        auto res = &mBuf[poppos];
        mPopPos.fetch_add(1, std::memory_order_acq_rel);
        return res;
    }
};
}
