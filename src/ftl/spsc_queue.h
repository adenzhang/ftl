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
template<class T, class Delete = std::default_delete<T>>
class SPSCRingQueue
{
    Delete mDel;
    const size_t mCap = 0;
    T *mBuf = nullptr;
    std::atomic<size_t> mPushPos = 0, mPopPos = 0;

    SPSCRingQueue( const SPSCRingQueue & ) = delete;
    SPSCRingQueue &operator=( const SPSCRingQueue & ) = delete;

public:
    SPSCRingQueue( size_t cap, T *buf, Delete &&del = Delete() ) : mDel( del ), mCap( cap ), mBuf( buf )
    {
    }
    SPSCRingQueue( SPSCRingQueue &&a ) : mDel( a.mDel ), mCap( a.mCap ), mBuf( a.mBuf ), mPushPos( a.mPushPos ), mPopPos( a.mPopPos )
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
        if ( mBuf )
        {
            mDel( mBuf );
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

    template<class... Args>
    T *push( Args &&... args )
    {
        if ( !mCap )
            return nullptr;
        auto pushpos = mPushPos.load( std::memory_order_relaxed ) % mCap;
        auto poppos = mPopPos.load( std::memory_order_acquire ) % mCap;
        if ( ( pushpos + 1 ) % mCap == poppos ) // full
            return nullptr;
        new ( mBuf + pushpos ) T( std::forward<Args>( args )... );
        mPushPos.fetch_add( 1, std::memory_order_acq_rel );
        return mBuf + pushpos;
    }

    T *top()
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
