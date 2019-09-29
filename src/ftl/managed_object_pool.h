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

#include <cassert>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>
#include <atomic>

#define USE_DELETERADAPTOR

/// Pool-managed objects are created by pools. Users call pObj->delete_me() to return to pool.
/// Pools call pObj->set_deallocator(deallocator) after creating objects.
/// If set_deallocator is not called, delete_me will do nothing.
///
/// classes for deletable:
///     - StaticFnDeleter<T> : Deleter calls static function pFunc(void *pObj, T*>.
///     - Detetable<T> : base class of object T that can be allocated by pool.
/// classes for allocators/pools :
///     - FreeList: thread-unsafe free list.
///     - ConcurrentFreeList: lock-free free list.
///     - FixedPool<T, Alloc, FreeListT> : fix-size pool.
///     - ChunkPool<T, GrowthStrategyT, Alloc, FreeListT>.
///     -
namespace ftl
{

/// @brief StaticFnDeleter call fn(D*, T*) to delete object.
template<class T>
struct StaticFnDeleter
{
    using D = void *;
    using DelFn = void ( * )( D, T * );

    DelFn pDelFn = nullptr;
    D pDeleter = D();

    template<class DT>
    StaticFnDeleter( DelFn pF, DT pD ) : pDelFn( pF ), pDeleter( static_cast<D>( pD ) )
    {
    }

    StaticFnDeleter() = default;

    void deallocate( T *p )
    {
        assert( pDelFn );
        ( *pDelFn )( pDeleter, p );
    }

    void operator()( T *p )
    {
        deallocate( p );
    }

    static void default_delete_func( void *, T *p )
    {
        static std::default_delete<T> del;
        del( p );
    }
    static const StaticFnDeleter &get_default()
    {
        static StaticFnDeleter del( default_delete_func, D{} );
        return del;
    }
};
template<class ObjT>
class Deletable
{
public:
    using deallocator_type = StaticFnDeleter<ObjT>;

    void set_deallocator( typename StaticFnDeleter<ObjT>::DelFn fun, void *d )
    {
        mDel.pDelFn = fun;
        mDel.pDeleter = d;
    }
    void set_deallocator( const StaticFnDeleter<ObjT> &del )
    {
        mDel = del;
    }

    void delete_me()
    {
        if ( mDel.pDelFn )
        {
            auto pFun = mDel.pDelFn;
            mDel.pDelFn = nullptr;
            static_cast<ObjT>( this )->~ObjT();
            ( *pFun )( mDel.pDeleter, static_cast<ObjT *>( this ) );
        }
    }

    deallocator_type &get_deallocator()
    {
        return mDel;
    }

protected:
    StaticFnDeleter<ObjT> mDel;
};

template<class T>
struct null_delete
{
    void operator()( T * )
    {
    }
};

template<class T, class Deleter = std::default_delete<T>>
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

    static_assert( sizeof( T ) >= sizeof( FreeNode * ), "sizeof T < sizeof(void *)" );

    FreeList( T *p = nullptr, const Deleter &d = {} ) : pHead( reinterpret_cast<FreeNode *>( p ) ), mDel( d )
    {
    }

    FreeList( const FreeList &a ) : FreeList( nullptr, a.mDel )
    {
    }
    FreeList( FreeList &&a ) : mDel( std::move( a.mDel ) ), pHead( a.pHead ), mSize( a.mSize )
    {
        a.pHead = nullptr;
        a.mSize = 0;
    }

    FreeList( const Deleter &d ) : mDel( d )
    {
    }

    void push( T &p )
    {
        new ( reinterpret_cast<FreeNode *>( &p ) ) FreeNode( pHead );
        pHead = reinterpret_cast<FreeNode *>( &p );
        ++mSize;
    }

    T *pop()
    {
        T *res = reinterpret_cast<T *>( pHead );
        if ( pHead )
        {
            pHead = pHead->pNext;
            --mSize;
        }
        return res;
    }
    bool empty() const
    {
        return !pHead;
    }
    bool size() const
    {
        return mSize;
    }

    template<class D = Deleter>
    typename std::enable_if<std::is_same<D, null_delete<T>>::value, void>::type clear()
    {
        pHead = nullptr;
    }

    template<class D = Deleter>
    typename std::enable_if<!std::is_same<D, null_delete<T>>::value, void>::type clear()
    {
        while ( auto p = pop() )
        {
            mDel( p );
        }
        pHead = nullptr;
    }

    FreeNode *pHead = nullptr;
    size_t mSize = 0;
    Deleter mDel;
};

/// @brief Lockfree FreeList
template<class T, class Deleter = std::default_delete<T>>
struct ConcurrentFreeList
{
    struct FreeNode
    {
        union {
            char buf[sizeof( T )];
            std::atomic<FreeNode *> next;
        } data;

        void init()
        {
            new ( &data.next ) std::atomic<FreeNode *>();
        }

        T *get_object()
        {
            return reinterpret_cast<T *>( data.buf );
        }
        std::atomic<FreeNode *> &get_next_ref()
        {
            return data.next;
        }
        static FreeNode *from_object( T *pObj )
        {
            return reinterpret_cast<FreeNode *>( pObj );
        }
    };

    static_assert( sizeof( T ) >= sizeof( std::atomic<FreeNode *> ), "sizeof T < sizeof(std::atomic<FreeNode *> )" );

    ConcurrentFreeList( T *p = nullptr, const Deleter &d = {} ) : mHead( reinterpret_cast<FreeNode *>( p ) ), mDel( d )
    {
    }

    ConcurrentFreeList( const Deleter &d ) : mDel( d )
    {
    }

    ConcurrentFreeList( const ConcurrentFreeList &a ) : ConcurrentFreeList( nullptr, a.mDel )
    {
    }
    ConcurrentFreeList( ConcurrentFreeList &&a ) : ConcurrentFreeList( a.mHead.load(), std::move( a.mDel ) )
    {
        mSize = a.mSize;
        a.mHead = nullptr;
    }

    void push( T &obj )
    {
        auto pNode = FreeNode::from_object( &obj );
        pNode->init();
        FreeNode *pHead = nullptr;
        do
        {
            pHead = mHead.load();
            pNode->get_next_ref() = pHead;
        } while ( !mHead.compare_exchange_weak( pHead, pNode ) );

        ++mSize;
    }

    T *pop()
    {
        auto pNode = mHead.load();
        if ( pNode )
        {
            FreeNode *pNext = nullptr;
            do
            {
                pNode = mHead.load();
                if ( !pNode )
                {
                    ++mSize;
                    break;
                }
                pNext = pNode->data.next.load();
            } while ( !mHead.compare_exchange_weak( pNode, pNext ) );
            --mSize;
        }
        return pNode->get_object();
    }
    bool empty() const
    {
        return !mHead.load();
    }
    bool size() const
    {
        return mSize;
    }

    template<class D = Deleter>
    typename std::enable_if<std::is_same<D, null_delete<T>>::value, void>::type clear()
    {
        mHead = nullptr;
        mSize = 0;
    }

    template<class D = Deleter>
    typename std::enable_if<!std::is_same<D, null_delete<T>>::value, void>::type clear()
    {
        while ( auto p = pop() )
        {
            mDel( p );
        }
        mHead = nullptr;
        mSize = 0;
    }

    std::atomic<FreeNode *> mHead{nullptr};
    std::atomic<size_t> mSize{0};
    Deleter mDel;
};

/// @brief Allocate when constructing ManagedObjectPoolBase.
/// ChildT implements: T *do_alloc().
/// T require: either IDeletable or IDeallocatable.
/// ManagedObjectPoolBase is a IDeleter<T> and a IDeallocator<T>
template<class T, class FreeList, class ChildT>
class ManagedObjectPoolBase
{
public:
    static_assert( std::is_base_of_v<Deletable<T>, T>, "Deletable<T> must be base of T" );

    using child_type = ChildT;
    using this_type = ManagedObjectPoolBase;

    ManagedObjectPoolBase( size_t nReserve, const FreeList &fl ) : mFreeList( fl )
    {
        reserve( nReserve );
    }
    virtual ~ManagedObjectPoolBase() = default;

    /// @return new allocated count.
    size_t reserve( size_t nsize )
    {
        size_t n = 0;
        while ( mFreeList.size() < nsize )
        {
            auto p = reinterpret_cast<child_type *>( this )->do_alloc();
            if ( !p )
                break;
            mFreeList.push( *p );
            ++n;
        }
        return n;
    }

    FreeList &get_freelist()
    {
        return mFreeList;
    }

    T *allocate( size_t n = 1 )
    {
        assert( n == 1 );
        if ( n != 1 )
            return nullptr;
        if ( auto p = mFreeList.pop() )
            return p;
        else
        {
            return reinterpret_cast<child_type *>( this )->do_alloc();
        }
    }

    virtual T *do_alloc() = 0;

    template<class... Args>
    T *create( Args &&... args )
    {
        if ( auto p = allocate() )
        {
            new ( p ) T( std::forward<Args>( args )... );
            p->set_deallocator( &this_type::Deallocate, this );

            return p;
        }
        return nullptr;
    }

    template<class... Args>
    std::shared_ptr<T> create_shared( Args &&... args )
    {
        if ( auto p = allocate() )
        {
            new ( p ) T( std::forward<Args>( args )... );
            return {p, StaticFnDeleter<T>( this_type::Deallocate, this )}; // return {p, [this]( T *pObj ) { Deallocate( this, pObj ); }};
        }
        return nullptr;
    }

    /// @brief if create_unique is used, T::Ptr must be defined, and MemberFnDeleterAdaptor<T> must be the deleter.
    template<class... Args>
    typename T::Ptr create_unique( Args &&... args )
    {
        if ( auto p = allocate() )
        {
            new ( p ) T( std::forward<Args>( args )... );
            return {p, StaticFnDeleter<T>( this_type::Deallocate, this )}; // return {p, [this]( T *pObj ) { Deallocate( this, pObj ); }};
        }
        return nullptr;
    }

    static void Deallocate( void *d, T *p )
    {
        static_cast<this_type *>( d )->mFreeList.push( *p );
    }

    void clear() // reset
    {
        mFreeList.clear();
    }

protected:
    FreeList mFreeList;
};

/// @brief Allocate fix-size objects when contructing pool.
template<typename T, class Alloc = std::allocator<T>, class FreeList = ConcurrentFreeList<T, null_delete<T>>>
class FixedPool : ManagedObjectPoolBase<T, FreeList, FixedPool<T, Alloc, FreeList>>
{
public:
    using this_type = FixedPool;
    using parent_type = ManagedObjectPoolBase<T, FreeList, this_type>;

    using parent_type::create;
    using parent_type::create_shared;
    using parent_type::create_unique;

    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    FixedPool( size_t nBlocks, const Alloc &a = Alloc(), const FreeList &freeList = FreeList() )
        : parent_type( 0, freeList ), mAlloc( a ), mBlocks( nBlocks )
    {
        if ( !nBlocks )
            return;
        mCurrent = mMem = mAlloc.allocate( mBlocks );
        assert( mMem );
        if ( !mMem )
        {
            throw std::bad_alloc();
        }
    }

    ~FixedPool() override
    {
        if ( mMem )
        {
            mAlloc.deallocate( mMem, mBlocks );
            mMem = nullptr;
        }
    }

    T *do_alloc()
    {
        if ( mCurrent + 1 > mMem + mBlocks )
            return nullptr;
        auto res = ( mCurrent );
        mCurrent += 1;
        return res;
    }

    void clear() // reset
    {
        parent_type::clear();
        mCurrent = mMem;
    }

protected:
    Alloc mAlloc;
    const size_t mBlocks;
    T *mMem = nullptr;
    T *mCurrent;
};

struct ConstGrowthStrategy
{
    ConstGrowthStrategy( size_t initial ) : value( initial )
    {
    }
    size_t next_value()
    {
        return value;
    }
    const size_t value;
};

struct DoubleGrowthStrategy
{
    DoubleGrowthStrategy( size_t initial ) : value( initial )
    {
    }
    size_t next_value()
    {
        value *= 2;
        return value;
    }
    size_t value;
};

/// @brief Allocate objects by chunk.
template<typename T,
         class ChunkSizeGrowthStrategy = ConstGrowthStrategy,
         class Alloc = std::allocator<T>,
         class FreeList = ConcurrentFreeList<T, null_delete<T>>>
class ManagedChunkPool : ManagedObjectPoolBase<T, FreeList, ManagedChunkPool<T, ChunkSizeGrowthStrategy, Alloc, FreeList>>
{
public:
    using this_type = ManagedChunkPool;
    using parent_type = ManagedObjectPoolBase<T, FreeList, this_type>;

    using parent_type::create;
    using parent_type::create_shared;
    using parent_type::create_unique;

    static constexpr bool is_const_growth = std::is_same_v<ChunkSizeGrowthStrategy, ConstGrowthStrategy>;

    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    ManagedChunkPool( size_t chunckSize = 16, size_t initialChunks = 0, const Alloc &a = Alloc(), const FreeList &freeList = FreeList() )
        : parent_type( 0, freeList ), mAlloc( a ), mChuckSize( chunckSize ), mCurrChunk( 0 )
    {
        for ( size_t i = 0; i < initialChunks; ++i )
            append_chunk();
    }

    ~ManagedChunkPool()
    {
        for ( auto &ch : mChunks )
        {
            if constexpr ( is_const_growth )
                mAlloc.deallocate( ch.mMem, mChuckSize.next_value() );
            else
                mAlloc.deallocate( ch.mMem, ch.mSize );
            ch.clear();
        }
    }

    T *do_alloc()
    {
        while ( true )
        {
            if ( mCurrChunk >= mChunks.size() )
            {
                if ( auto pChunk = append_chunk() )
                {
                    if constexpr ( is_const_growth )
                        return pChunk->do_alloc( mChuckSize.next_value() );
                    else
                        return pChunk->do_alloc();
                }
                else
                    return nullptr;
            }
            else
            {
                auto &ch = mChunks[mCurrChunk];
                if constexpr ( is_const_growth )
                {
                    if ( auto p = ch.do_alloc( mChuckSize.next_value() ) )
                        return p;
                }
                else
                {
                    if ( auto p = ch.do_alloc() )
                        return p;
                }

                ++mCurrChunk;
            }
        }
    }

    void clear() // reset/free allocated objects
    {
        parent_type::clear();
        for ( auto &ch : mChunks )
        {
            ch.clear();
        }
        mCurrChunk = 0;
    }

protected:
    struct DynSizedChunkInfo
    {
        size_t mSize = 0;
        T *mMem = nullptr;
        T *mCurrent = nullptr;

        DynSizedChunkInfo( size_t n = 0 ) : mSize( n )
        {
        }

        void clear()
        {
            mCurrent = mMem;
        }
        T *do_alloc()
        {
            if ( mCurrent + 1 > mMem + mSize )
                return nullptr;
            auto res = ( mCurrent );
            mCurrent += 1;
            return res;
        }
    };

    struct ConstSizedChunkInfo
    {
        T *mMem = nullptr;
        T *mCurrent = nullptr;

        ConstSizedChunkInfo( size_t )
        {
        }

        void clear()
        {
            mCurrent = mMem;
        }
        T *do_alloc( size_t mSize )
        {
            if ( mCurrent + 1 > mMem + mSize )
                return nullptr;
            auto res = ( mCurrent );
            mCurrent += 1;
            return res;
        }
    };
    using ChunkInfo = std::conditional_t<is_const_growth, ConstSizedChunkInfo, DynSizedChunkInfo>;

    ChunkInfo *append_chunk()
    {
        size_t n = mChuckSize.next_value();
        ChunkInfo ch{n};
        ch.mCurrent = ch.mMem = mAlloc.allocate( n );
        if ( !ch.mMem )
            return nullptr;
        return &mChunks.emplace_back( ch );
    }
    using ChunkList = std::vector<ChunkInfo>;
    ChunkSizeGrowthStrategy mChuckSize;
    Alloc mAlloc;
    ChunkList mChunks;
    size_t mCurrChunk;
};

/// @brief Allocate one object when needed
template<typename T, class Alloc = std::allocator<T>, class FreeList = ConcurrentFreeList<T, std::default_delete<T>>>
class ManagedObjectPool : ManagedObjectPoolBase<T, FreeList, ManagedObjectPoolBase<T, Alloc, FreeList>>
{
public:
    using this_type = ManagedObjectPool;
    using parent_type = ManagedObjectPoolBase<T, FreeList, this_type>;

    using parent_type::create;
    using parent_type::create_shared;
    using parent_type::create_unique;

    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    ManagedObjectPool( size_t nReserve = 0, const Alloc &a = Alloc(), const FreeList &freeList = FreeList() )
        : parent_type( nReserve, freeList ), mAlloc( a )
    {
    }

    T *do_alloc()
    {
        return mAlloc.allocate( 1 );
    }

    void clear() // reset
    {
        parent_type::clear();
    }

protected:
    Alloc mAlloc;
};

} // namespace ftl
