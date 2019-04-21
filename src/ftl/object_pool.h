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

// Object Pool with lockfree free list
template<class T, class AllocT = std::allocator<T>>
class ObjectPool
{
public:
    template<class U>
    struct FreeNode
    {
        union {
            char buf[sizeof( U )];
            std::atomic<FreeNode *> next;
        } data;

        void Init()
        {
            new ( &data.next ) std::atomic<FreeNode *>();
        }

        T *GetObject()
        {
            return reinterpret_cast<T *>( data.buf );
        }
        std::atomic<FreeNode *> &GetNextRef()
        {
            return data.next;
        }
        static FreeNode *FromObject( T *pObj )
        {
            return reinterpret_cast<FreeNode *>( pObj );
        }
    };

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
            pNode->Init();
            pNode->data.next = mFreeList.load();
            mFreeList = pNode;
            ++mAllocated;
            ++mFreeSize;
        }
    }
    ~ObjectPool()
    {
        assert( mAllocated == mFreeSize );
        while ( mFreeList.load() )
        {
            auto p = mFreeList.load()->GetNextRef().load();
            mAlloc.deallocate( p, 1 );
            mFreeList = p;
        }
    }

    size_t CountAllocated() const
    {
        return mAllocated;
    }
    size_t CountFree() const
    {
        return mFreeSize;
    }

    template<class... Args>
    T *GetOrCreateObject( Args &&... args )
    {
        auto pNode = mFreeList.load();
        if ( pNode )
        {
            FreeNodeType *pNext = nullptr;
            do
            {
                pNode = mFreeList.load();
                if ( !pNode )
                    break;
                pNext = pNode->data.next.load();

            } while ( !mFreeList.compare_exchange_weak( pNode, pNext ) );

            if ( pNode )
                --mFreeSize;
        }
        if ( !pNode )
        {
            pNode = mAlloc.allocate( 1 );
            if ( pNode )
                ++mAllocated;
        }
        if ( pNode )
        {
            new ( pNode->GetObject() ) T( std::forward<Args...>( args )... );
            return pNode->GetObject();
        }
        return nullptr;
    }

    // put object into free list without checking whether it's allocated by pool or not
    void ReleaseObject( T *pObj )
    {
        if ( !pObj )
            return;
        pObj->~T();
        auto pNode = FreeNodeType::FromObject( pObj );
        pNode->Init();
        FreeNodeType *pHead = nullptr;
        do
        {
            pHead = mFreeList.load();
            pNode->GetNextRef() = pHead;
        } while ( !mFreeList.compare_exchange_weak( pHead, pNode ) );
        ++mFreeSize;
    }
};
} // namespace ftl
