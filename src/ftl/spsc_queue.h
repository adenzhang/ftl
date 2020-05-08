/*
 * This file is part of the ftl (Fast Template Library) distribution (https://github.com/adenzhang/ftl).
 * Copyright (c) 2018 Aden Zhang.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>
#include <memory>

namespace ftl
{

// lockfree SPSCRingQueue
template<class T, class AllocT = std::allocator<T>>
class SPSCRingQueue
{
    AllocT mAlloc;
    const size_t mCap = 0;
    T *mBuf = nullptr;
    std::atomic<size_t> mPushPos = 0, mPopPos = 0;

public:
    constexpr static bool support_multiple_producer_threads = false, support_multiple_consumer_threads = false;

    SPSCRingQueue &operator=( const SPSCRingQueue & ) = delete;

public:
    SPSCRingQueue( size_t cap = 0, const AllocT &alloc = AllocT() ) : mAlloc( alloc )
    {
        if ( cap )
            init( cap );
    }

    SPSCRingQueue( SPSCRingQueue &&a ) : mAlloc( a.mAlloc ), mCap( a.mCap ), mBuf( a.mBuf ), mPushPos( a.mPushPos ), mPopPos( a.mPopPos )
    {
        a.mBuf = nullptr;
        a.mPushPos = 0;
        a.mPopPos = 0;
        if ( mBuf == nullptr && mCap > 0 )
            init( mCap );
    }

    SPSCRingQueue( const SPSCRingQueue &a ) : mAlloc( a.alloc )
    {
        init( mCap );
    }

    bool init( size_t cap )
    {
        destruct();
        mCap = cap;
        mPushPos = 0;
        mPopPos = 0;
        if ( mCap > 0 )
        {
            mBuf = mAlloc.allocate( mCap );
            return mBuf;
        }
        else
            return true;
    }

    ~SPSCRingQueue()
    {
        destruct();
    }

    void destruct()
    {
        if ( mBuf )
        {
            mAlloc.deallocate( mBuf, mCap );
            mBuf = nullptr;
        }
    }

    size_t capacity() const
    {
        return mCap - 1;
    }
    bool full() const
    {
        return mCap == 0 || ( mPushPos.load( std::memory_order_relaxed ) + 1 ) % mCap == mPopPos.load( std::memory_order_relaxed );
    }
    bool empty() const
    {
        return mCap == 0 || mPushPos.load( std::memory_order_relaxed ) == mPopPos.load( std::memory_order_relaxed );
    }
    size_t size() const
    {
        auto pushpos = mPushPos.load( std::memory_order_relaxed ) % mCap;
        auto poppos = mPopPos.load( std::memory_order_relaxed ) % mCap;
        return pushpos >= poppos ? ( pushpos - poppos ) : ( mCap - poppos + pushpos );
    }

    // get the position for push operation.
    T *prepare_push() const
    {
        if ( !mCap )
            return nullptr;
        auto pushpos = mPushPos.load( std::memory_order_relaxed ) % mCap;
        auto poppos = mPopPos.load( std::memory_order_acquire ) % mCap;
        if ( ( pushpos + 1 ) % mCap == poppos ) // full
            return nullptr;
        return mBuf + pushpos;
    }
    // prepare_push must be called before commit_push
    void commit_push()
    {
        mPushPos.fetch_add( 1, std::memory_order_acq_rel );
    }

    template<class... Args>
    T *emplace( Args &&... args )
    {
        if ( auto p = prepare_push() )
        {
            new ( p ) T( std::forward<Args>( args )... );
            commit_push();
            return p;
        }
        return nullptr;
    }
    T *push( const T &val )
    {
        return emplace( val );
    }

    T *top() const
    {
        if ( !mCap )
            return nullptr;
        auto pushpos = mPushPos.load( std::memory_order_acquire ) % mCap;
        auto poppos = mPopPos.load( std::memory_order_relaxed ) % mCap;
        if ( pushpos == poppos ) // empty
            return nullptr;
        return &mBuf[poppos];
    }
    // v: uninitialized memory
    bool pop( T *buf = nullptr )
    {
        if ( !mCap )
            return false;
        auto pushpos = mPushPos.load( std::memory_order_acquire ) % mCap;
        auto poppos = mPopPos.load( std::memory_order_relaxed ) % mCap;
        if ( pushpos == poppos ) // empty
            return false;
        if ( buf )
            new ( buf ) T( std::move( mBuf[poppos] ) );
        mBuf[poppos].~T();
        mPopPos.fetch_add( 1, std::memory_order_acq_rel );
        return true;
    }
};
} // namespace ftl
