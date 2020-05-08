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
#include <ftl/chunk_allocator.h>

/////////////////////////////////////////////////////////////////////////////////
///    Discarded this file
/////////////////////////////////////////////////////////////////////////////////

namespace ftl
{

/////////////////////////////////////////////////////////////////////////////////
///    FreeList, AtomicFreeList
/////////////////////////////////////////////////////////////////////////////////


// @brief FreeList traits:
///    - typename Node
///    - FreeList(FreeList&& )
///    - T *pop()
///    - void push(T &)
///    - bool empty() const
//     - void clear()
template<class T>
struct FreeList
{
    struct FreeNode
    {
        FreeNode *pNext;

        FreeNode( T *next = nullptr ) : pNext( reinterpret_cast<FreeNode *>( next ) )
        {
        }
        FreeNode( FreeNode *next = nullptr ) : pNext( next )
        {
        }
    };
    using Node = FreeNode;

    // static_assert( sizeof( T ) >= sizeof( FreeNode * ), "sizeof T < sizeof(void *)" );

    FreeList() = default;
    FreeList( FreeList &&a )
    {
        pHead = a.pHead;
        a.pHead = nullptr;
    }

    void push( T &p )
    {
        new ( reinterpret_cast<FreeNode *>( &p ) ) FreeNode( pHead );
        pHead = reinterpret_cast<FreeNode *>( &p );
    }

    T *pop()
    {
        T *res = reinterpret_cast<T *>( pHead );
        if ( pHead )
            pHead = pHead->pNext;
        return res;
    }
    bool empty() const
    {
        return !pHead;
    }
    // release head / all.
    void clear()
    {
        pHead = nullptr;
    }
    // FreeList(const FreeList&) = delete;
    FreeNode *pHead = nullptr;
};

template<class U>
struct AtomicFreeList
{
    struct AtomicFreeNode
    {
        using this_type = AtomicFreeNode;
        using Ptr = std::atomic<AtomicFreeNode *>;

        static_assert( sizeof( U ) >= sizeof( std::atomic<AtomicFreeNode *> ), "Object U is too small!" );

        union {
            char buf[sizeof( U )];
            Ptr next;
        } data;

        void init()
        {
            new ( &data.next ) Ptr();
        }

        U *get_object()
        {
            return reinterpret_cast<U *>( data.buf );
        }
        Ptr &get_next_ref()
        {
            return data.next;
        }

        static void push( Ptr &head, U *pObj )
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

        static U *pop( Ptr &head )
        {
            auto pHead = head.load();
            this_type *pNext = nullptr;
            do
            {
                if ( !pHead )
                    break;
                pNext = pHead->data.next.load();
            } while ( !head.compare_exchange_weak( pHead, pNext ) );
            return pHead;
        }

        static AtomicFreeNode *from_object( U *pObj )
        {
            return reinterpret_cast<AtomicFreeNode *>( pObj );
        }
    };

    using Node = AtomicFreeNode;

    AtomicFreeList() = default;
    AtomicFreeList( AtomicFreeList &&a )
    {
        m_pHead = a.pHead;
        a.pHead = nullptr;
    }
    void push( U &p )
    {
        AtomicFreeNode::push( &m_pHead, &p );
    }
    U *pop()
    {
        return AtomicFreeNode::pop( &m_pHead );
    }
    bool empty() const
    {
        return nullptr == m_pHead.load();
    }
    void clear()
    {
        m_pHead = nullptr;
    }

    std::atomic<Node *> m_pHead;
};
// Object Pool with lockfree free list
template<class T, class AllocT = ChunkAllocator<T>, typename FreeList = AtomicFreeList<T>>
class ObjectPool
{
public:
    using FreeNodeType = typename FreeList::Node;
    using Alloc = typename std::allocator_traits<AllocT>::template rebind_alloc<FreeNodeType>;
    using this_type = ObjectPool;

    struct Deleter
    {
        ObjectPool &alloc;
        Deleter( ObjectPool &a ) : alloc( a )
        {
        }

        void operator()( T *p )
        {
            if ( p )
            {
                p->~T();
                alloc.deallocate( p );
            }
        }
    };

    struct Allocator
    {
        using value_type = T;
        ObjectPool &alloc;
        Allocator( ObjectPool &a ) : alloc( a )
        {
        }
        T *allocate( size_t = 1 )
        {
            return alloc.allocate();
        }
        void deallocate( T *p, size_t = 1 )
        {
            alloc.deallocate( p, 1 );
        }
        void clear()
        {
            alloc.clear();
        }
    };

protected:
    Alloc mAlloc;
    FreeList mFreeList = 0;
    std::atomic<size_t> mAllocated = 0, mFreeSize = 0;

public:
    ObjectPool( size_t initialAllocSize = 0, const AllocT &alloc = AllocT{} ) : mAlloc( alloc )
    {
        for ( size_t i = 0; i < initialAllocSize; ++i )
        {
            auto pNode = mAlloc.allocate( 1 );
            assert( pNode );
            mFreeList.push( pNode );
            ++mAllocated;
            ++mFreeSize;
        }
    }
    ~ObjectPool()
    {
        assert( mAllocated == mFreeSize );
        T *pNode = nullptr;
        while ( ( pNode = mFreeList.pop() ) )
            mAlloc.deallocate( pNode, 1 );
    }

    size_t allocated_size() const
    {
        return mAllocated;
    }
    size_t free_size() const
    {
        return mFreeSize;
    }

    T *allocate( size_t = 1 )
    {
        auto pNode = mFreeList.pop();
        if ( pNode )
        {
            --mFreeSize;
            return pNode;
        }
        pNode = mAlloc.allocate( 1 );
        if ( pNode )
            ++mAllocated;
        return pNode;
    }

    template<class... Args>
    T *create( Args &&... args )
    {
        auto pNode = allocate();
        if ( pNode )
            new ( pNode ) T( std::forward<Args...>( args )... );
        return pNode;
    }

    template<class... Args>
    std::unique_ptr<T, Deleter> create_unique( Args &&... args )
    {
        return std::unique_ptr<T, Deleter>( create( std::forward<Args>( args )... ), to_deleter() );
    }

    void destroy( T *pObj )
    {
        if ( !pObj )
            return;
        pObj->~T();
        deallocate( pObj );
    }

    // put object into free list without checking whether it's allocated by pool or not
    void deallocate( T *pObj, size_t = 1 )
    {
        if ( !pObj )
            return;
        mFreeList.push( pObj );
        ++mFreeSize;
    }

    void clear()
    {
        Alloc::clear();
    }

    Deleter to_deleter() const
    {
        return Deleter( *this );
    }
    Allocator to_allocator() const
    {
        return Allocator( *this );
    }
    Alloc &get_allocator()
    {
        return mAlloc;
    }
};
} // namespace ftl
