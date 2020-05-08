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
#include <optional>

namespace ftl
{

struct AtomicNextNodeBase
{
    std::atomic<AtomicNextNodeBase *> pNext = nullptr;
};

// unbounded lockfree generic MPSCQueue
class MPSCUnboundedNodeQueue
{

    AtomicNextNodeBase mStub; // tail
    std::atomic<AtomicNextNodeBase *> mHead; // push to head, pop from tail
    AtomicNextNodeBase *mTail = nullptr;

    std::atomic<size_t> mSize = 0;

    MPSCUnboundedNodeQueue( const MPSCUnboundedNodeQueue & ) = delete;
    MPSCUnboundedNodeQueue &operator=( const MPSCUnboundedNodeQueue & ) = delete;

public:
    MPSCUnboundedNodeQueue() : mHead( &mStub ), mTail( &mStub )
    {
    }

    ~MPSCUnboundedNodeQueue()
    { // allow destructing non-empty queue
    }

    void push( AtomicNextNodeBase *pNode )
    {
        pNode->pNext.store( nullptr, std::memory_order_relaxed );
        auto phead = mHead.exchange( pNode, std::memory_order_acq_rel );
        phead->pNext.store( pNode, std::memory_order_release );
        mSize.fetch_add( 1 );
    }

    size_t size() const
    {
        return mSize.load();
    }
    bool empty() const
    {
        return size() == 0;
    }

    AtomicNextNodeBase *top()
    {
        if ( mTail == &mStub ) // possibly empty
        {
            if ( auto pnext = mTail->pNext.load( std::memory_order_acquire ) )
                mTail = pnext; // update tail
            else
                return nullptr; // empty
        }
        return mTail;
    }
    AtomicNextNodeBase *pop()
    {
        if ( mTail == &mStub ) // possibly empty
        {
            if ( auto pnext = mTail->pNext.load( std::memory_order_acquire ) )
                mTail = pnext; // update tail
            else
                return nullptr; // empty
        }
        auto tail = mTail;
        if ( !mTail->pNext.load( std::memory_order_acquire ) ) // only 1 elements
            push( &mStub ); // push mStub, so that mHead is updated
        mTail = tail->pNext.load( std::memory_order_acquire ); // now mTail may or may not point to mStub
        mSize.fetch_sub( 1 );
        return tail;
    }
};


// unbounded lockfree MPSCQueue
template<class T, class Alloc = std::allocator<T>>
class MPSCUnboundedQueue
{

    struct Node : AtomicNextNodeBase
    {
        T val;
    };
    using allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;

    MPSCUnboundedNodeQueue mQue;
    Alloc mAlloc;

    MPSCUnboundedQueue( const MPSCUnboundedQueue & ) = delete;
    MPSCUnboundedQueue &operator=( const MPSCUnboundedQueue & ) = delete;

public:
    MPSCUnboundedQueue( Alloc &&alloc = Alloc() ) : mQue(), mAlloc( alloc )
    {
    }
    ~MPSCUnboundedQueue()
    {
        while ( pop() )
            ;
    }

    template<class... Args>
    bool emplace( Args &&... args )
    {
        auto pNode = mAlloc.allocate( 1 );
        new ( &pNode->val ) T( std::forward<Args>( args )... );
        mQue.push( pNode );
        return true;
    }
    bool push( const T &val )
    {
        return emplace( val );
    }

    size_t size() const
    {
        return mQue.size();
    }
    bool empty() const
    {
        return mQue.empty();
    }

    T *top()
    {
        if ( auto pNode = static_cast<Node *>( mQue.top() ) )
        {
            return &pNode->val;
        }
        return nullptr;
    }
    // buf: uninitialized memory
    bool pop( T *buf = nullptr )
    {
        if ( auto pNode = static_cast<Node *>( mQue.pop() ) )
        {
            if ( buf )
                new ( buf ) T( std::move( pNode->val ) );
            pNode->val.~T();
            mAlloc.deallocate( pNode );
            return true;
        }
        return false;
    }
};
} // namespace ftl
