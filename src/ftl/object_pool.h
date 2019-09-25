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
#include <memory>
#include <atomic>
#include <cassert>

namespace ftl
{

template<class U>
struct FreeNode
{
    using this_type = FreeNode;

    static_assert( sizeof( U ) >= sizeof( std::atomic<FreeNode *> ), "Object U is too small!" );

    union {
        char buf[sizeof( U )];
        std::atomic<FreeNode *> next;
    } data;

    void init()
    {
        new ( &data.next ) std::atomic<FreeNode *>();
    }

    U *get_object()
    {
        return reinterpret_cast<U *>( data.buf );
    }
    std::atomic<FreeNode *> &get_next_ref()
    {
        return data.next;
    }


    static void push( std::atomic<this_type *> &head, U *pObj )
    {
        auto pNode = from_object( pObj );
        pNode->init();
        this_type *pHead = nullptr;
        do
        {
            pHead = head.load();
            pNode->get_next_ref() = pHead;
        } while ( !head.compare_exchange_weak( pHead, pNode ) );
    }

    static U *pop( std::atomic<this_type *> &head )
    {
        auto pNode = head.load();
        if ( pNode )
        {
            this_type *pNext = nullptr;
            do
            {
                pNode = head.load();
                if ( !pNode )
                    break;
                pNext = pNode->data.next.load();

            } while ( !head.compare_exchange_weak( pNode, pNext ) );
        }
        return pNode;
    }

    static FreeNode *from_object( U *pObj )
    {
        return reinterpret_cast<FreeNode *>( pObj );
    }
};

// Object Pool with lockfree free list
template<class T, class AllocT = std::allocator<T>>
class ObjectPool
{
public:
    using FreeNodeType = FreeNode<T>;
    using Alloc = typename std::allocator_traits<AllocT>::template rebind_alloc<FreeNodeType>;

protected:
    Alloc mAlloc;
    std::atomic<FreeNodeType *> mFreeList = 0;

    std::atomic<size_t> mAllocated = 0, mFreeSize = 0;

public:
    ObjectPool( size_t initialAllocSize = 0, const AllocT &alloc = AllocT{} ) : mAlloc( alloc )
    {
        for ( size_t i = 0; i < initialAllocSize; ++i )
        {
            auto pNode = mAlloc.allocate( 1 );
            assert( pNode );
            FreeNodeType::push( mFreeList, pNode );
            ++mAllocated;
            ++mFreeSize;
        }
    }
    ~ObjectPool()
    {
        assert( mAllocated == mFreeSize );
        while ( mFreeList.load() )
        {
            auto p = mFreeList.load()->get_next_ref().load();
            mAlloc.deallocate( p, 1 );
            mFreeList = p;
        }
    }

    size_t allocated_size() const
    {
        return mAllocated;
    }
    size_t free_size() const
    {
        return mFreeSize;
    }

    template<class... Args>
    T *get_object( Args &&... args )
    {
        auto pNode = FreeNodeType::pop( mFreeList );
        if ( pNode )
        {
            new ( pNode->get_object() ) T( std::forward<Args...>( args )... );
            return pNode->get_object();
        }
        return nullptr;
    }

    template<class... Args>
    T *get_or_create_object( Args &&... args )
    {
        auto pNode = FreeNodeType::pop( mFreeList );
        if ( !pNode )
        {
            pNode = mAlloc.allocate( 1 );
            if ( pNode )
                ++mAllocated;
        }
        if ( pNode )
        {
            new ( pNode->get_object() ) T( std::forward<Args...>( args )... );
            return pNode->get_object();
        }
        return nullptr;
    }

    // put object into free list without checking whether it's allocated by pool or not
    void release_object( T *pObj )
    {
        if ( !pObj )
            return;
        pObj->~T();
        FreeNodeType::push( mFreeList, pObj );
        ++mFreeSize;
    }
};
} // namespace ftl
