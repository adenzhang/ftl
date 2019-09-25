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
#include <cassert>
#include <memory>
#include <optional>

namespace ftl
{

// MPSC lock free bounded array queue
template<class T, class Alloc = std::allocator<T>, size_t Align = 64>
class MPSCBoundedQueue
{
public:
    using size_type = ptrdiff_t; // signed
    using seq_type = std::atomic<size_type>;
    struct Entry
    {
        typename std::aligned_storage<sizeof( seq_type ), Align>::type mSeq;
        typename std::aligned_storage<sizeof( T ), Align>::type mData;

        seq_type &seq()
        {
            return *reinterpret_cast<seq_type *>( &mSeq );
        }
        T &data()
        {
            return *reinterpret_cast<T *>( &mData );
        }
    };
    using allocator_type = Alloc;
    using node_allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<Entry>;

protected:
    node_allocator mAlloc;
    size_t mCap = 0;
    Entry *mBuf = nullptr;

    seq_type mPushPos = 0, mPopPos = 0;

public:
    using value_type = T;

    MPSCBoundedQueue( size_t cap = 0, allocator_type &&alloc = allocator_type() ) : mAlloc( std::move( alloc ) ), mCap( cap )
    {
        init( cap );
    }

    MPSCBoundedQueue( size_t cap, const allocator_type &alloc ) : mAlloc( alloc ), mCap( cap )
    {
        init( cap );
    }

    ~MPSCBoundedQueue()
    {

        if ( mBuf )
        {
            while ( top() )
                pop();
            mAlloc.deallocate( mBuf, mCap );
            mBuf = nullptr;
        }
    }

    // @brief Move Constructor: move memory if it's allocated. Otherwise, allocate new buffer.
    MPSCBoundedQueue( MPSCBoundedQueue &&a )
        : mAlloc( std::move( a.mAlloc ) ), mCap( a.mCap ), mBuf( a.mBuf ), mPushPos( a.mPushPos.load() ), mPopPos( a.mPopPos.load() )
    {
        a.mBuf = nullptr;
        if ( mBuf == nullptr && mCap > 0 )
            init( mCap );
    }

    // @brief Copy Constructor: alway allocate new buffer.
    MPSCBoundedQueue( const MPSCBoundedQueue &a ) : MPSCBoundedQueue( a.mCap, a.mAlloc )
    {
    }

    MPSCBoundedQueue &operator=( const MPSCBoundedQueue &a )
    {
        this->~MPSCBoundedQueue();
        new ( this ) MPSCBoundedQueue( a );
        return *this;
    }

    MPSCBoundedQueue &operator=( MPSCBoundedQueue &&a )
    {
        this->~MPSCBoundedQueue();
        new ( this ) MPSCBoundedQueue( std::move( a ) );
        return *this;
    }

    bool init( size_t cap )
    {
        this->~MPSCBoundedQueue();
        mCap = cap;
        mBuf = mAlloc.allocate( cap );
        if ( !mBuf )
            return false;
        for ( size_t i = 0; i < cap; ++i )
        {
            new ( &mBuf[i].seq() ) seq_type();
            mBuf[i].seq() = i;
        }
        mPushPos = 0;
        mPopPos = 0;
        return true;
    }

    // @return false if it's full.
    template<class... Args>
    bool push( Args &&... args )
    {
        for ( ;; )
        {
            auto pushpos = mPushPos.load( std::memory_order_relaxed );
            auto &entry = mBuf[pushpos % mCap];
            auto seq = entry.seq().load( std::memory_order_acquire );
            size_type diff = seq - pushpos;

            if ( diff == 0 )
            { // acquired pushpos & seq
                if ( mPushPos.compare_exchange_weak( pushpos, pushpos + 1, std::memory_order_relaxed ) )
                {
                    new ( &entry.data() ) T( std::forward<Args>( args )... );
                    entry.seq().store( seq + 1, std::memory_order_release ); // inc seq. when popping, seq == poppos+1
                    return true;
                } // else pushpos was acquired by other push thread, retry
            }
            else if ( diff < 0 )
            { // full
                return false;
            } // else pushpos was acquired by other push thread, retry
        }
    }

    // @brief Call T(T&&) to construct object T.
    // @return false if it's empty.
    bool pop_back( T *buf = nullptr )
    {
        return pop( buf );
    }

    // buf: uninitialized memory
    bool pop( T *buf = nullptr )
    {
        auto poppos = mPopPos.load( std::memory_order_acquire );
        auto &entry = mBuf[poppos % mCap];
        auto seq = entry.seq().load( std::memory_order_acquire );
        auto diff = seq - poppos - 1;

        if ( diff == 0 )
        {
            mPopPos.store( poppos + 1, std::memory_order_relaxed );
            if ( buf )
                new ( buf ) T( std::move( entry.data() ) );
            entry.data().~T();
            entry.seq().store( seq - 1 + mCap ); // dec seq and plus mCap. when next push, seq == pushpos.
            return true;
        }
        else
        { // empty
            assert( diff < 0 );
            return false;
        }
    }

    T *top() const
    {
        auto poppos = mPopPos.load( std::memory_order_acquire );
        auto &entry = mBuf[poppos % mCap];
        auto seq = entry.seq().load( std::memory_order_acquire );
        auto diff = seq - poppos - 1;

        if ( diff == 0 )
        {
            return &entry.data();
        }
        else
        { // empty
            assert( diff < 0 );
            return nullptr;
        }
    }

    std::size_t size() const
    {
        auto poppos = mPopPos.load( std::memory_order_relaxed );
        auto pushpos = mPushPos.load( std::memory_order_relaxed );
        if ( pushpos < poppos )
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
} // namespace ftl
