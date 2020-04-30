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
#include <ftl/alloc_common.h>
#include <ftl/sys_alloc.h>

namespace ftl
{

struct AllocRequest
{
    std::size_t slotSize = 0;
    std::size_t minSlotsPerSlab;

    std::size_t slotAlignment = 128; // default: 2 cache line size, avoiding false sharing.

    std::size_t slabAlignment = 8; // which is also the SlabHeader alignment.
    std::size_t slabGranularity = 4096; // page size.
    bool alignupToSlabGranularity = false; // if true, may pre-allocate more slots than requested.
    std::size_t maxSlotsPerSlab = 0; // applicable only if ConstGrowthStrategy is not true.
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief MemPool preallocates slots. User calls MemPool::malloc() to allocate a slot.
/// Multiple slots are allocated in a slab. Multiple slabs are linked into a list.
/// \tparam IsAtomic if true, The Memory pool is thread-safe.
/// \tparam ConstGrowthStrategy if false, count of slots in each new slab is double of previous value. Otherwise, always allocate the same size of
/// slab.
template<bool IsAtomic, bool ConstGrowthStrategy = true>
class MemPool
{
protected:
    struct SlabInfo
    {
        using size_type = unsigned;
        unsigned slabSize, firstSlotOffset, slotSize, slotCount;
    };

    // slabs are linked into list. Each slab contains multiple slots which are can allocated by calling MemPool::malloc().
    // slab 0: |SlabHeader|..|slot 1|slot 2|....|
    //            |
    // slab 1: |SlabHeader|..|slots ...|
    // other slabs ....
    struct SlabHeader
    {
        using NextElemPtrType = std::conditional_t<IsAtomic, std::atomic<SlabHeader *>, SlabHeader *>;
        NextElemPtrType pNext{};
        SlabInfo info;

        SlabHeader() = default;
        SlabHeader( const SlabInfo &info ) : info( info )
        {
        }
    };

    struct SlotHeader
    {
        using NextElemPtrType = std::conditional_t<IsAtomic, std::atomic<SlotHeader *>, SlotHeader *>;
        NextElemPtrType pNext{};
    };

    SlotHeader m_freeList;
    SlabHeader m_slabList;

    AllocRequest m_defaultAllocReq;

    using IntCounterType = std::conditional_t<IsAtomic, std::atomic<std::ptrdiff_t>, std::ptrdiff_t>;
    IntCounterType m_totalSlots = 0, m_allocatedSlots = 0; // for tracking allocations.


public:
    MemPool() = default;

    // @param slotSize size of each slot which is allocated by calling malloc().
    // @param nimSlotsPerSlab indicates min number of slots per slot.
    // @param slabGranuity usually multiple of page size (4096).
    MemPool( const AllocRequest &ar )
    {
        init( ar );
    }

    int init( const AllocRequest &ar )
    {
        m_defaultAllocReq = ar;
        return allocate_slab( ar );
    }

    // user may directly call allocate_slab to allocate a slab of slots.
    // @return number of slots allocated. -1 for error.
    int allocate_slab( const AllocRequest &r )
    {
        SlabInfo slabInfo;
        if ( !populateSlabInfo( slabInfo, r, sizeof( SlabHeader ) ) )
            return -1;

        auto pSlab = ftl::sys_aligned_reserve_and_commit( slabInfo.slabSize, r.slabAlignment, r.slabGranularity ); // may use std::aligned_allocate
        if ( !pSlab ) // failed allocatation.
            return -1;
        auto ret = segregate_slab( pSlab, slabInfo, m_slabList.pNext, m_freeList.pNext );
        assert( ret > 0 );
        if ( ret > 0 )
            m_totalSlots += std::ptrdiff_t( ret );
        return ret;
    }

    ~MemPool()
    {
        //        assert( free_size() == capacity() );
        while ( auto p = ftl::PopSinglyListNode<SlabHeader, &SlabHeader::pNext>( &m_slabList.pNext ) )
        {
            ftl::sys_release( p, p->info.slabSize );
        }
    }

    bool inited() const
    {
        return m_defaultAllocReq.slotSize;
    }

    /// \brief allocate a slot.
    /// \pre inited() successfully.
    void *malloc()
    {
        assert( inited() );
        auto p = ftl::PopSinglyListNode<SlotHeader, &SlotHeader::pNext>( &m_freeList.pNext );
        if ( !p )
        {
            // slow path, auto allocate a slab
            if constexpr ( !ConstGrowthStrategy )
            {
                /// \note not thread safe
                m_defaultAllocReq.minSlotsPerSlab = m_totalSlots * 2;
                if ( m_defaultAllocReq.maxSlotsPerSlab && m_defaultAllocReq.minSlotsPerSlab > m_defaultAllocReq.maxSlotsPerSlab )
                    m_defaultAllocReq.minSlotsPerSlab = m_defaultAllocReq.maxSlotsPerSlab;
            }
            allocate_slab( m_defaultAllocReq );
            p = ftl::PopSinglyListNode<SlotHeader, &SlotHeader::pNext>( &m_freeList.pNext );
        }
        if ( p )
        {
            m_allocatedSlots += 1;
            assert( m_allocatedSlots >= 0 && m_allocatedSlots <= m_totalSlots );
            return p;
        }
        return nullptr;
    }

    /// \brief free an allocated slot.
    void free( void *p )
    {
        m_allocatedSlots -= 1;
        assert( m_allocatedSlots >= 0 && m_allocatedSlots <= m_totalSlots );
        ftl::PushSinglyListNode<SlotHeader, &SlotHeader::pNext>( &m_freeList.pNext, reinterpret_cast<SlotHeader *>( p ) );
    }

    std::size_t capacity() const
    {
        return m_totalSlots;
    }

    std::size_t free_size() const
    {
        return m_totalSlots - m_allocatedSlots;
    }

    void clear()
    {
        new ( &m_freeList ) SlotHeader();
        while ( auto pSlabHeader = ftl::PopSinglyListNode<SlabHeader, &SlabHeader::pNext>( &m_slabList.pNext ) )
        {
            std::size_t k = 0;
            for ( Byte *pSlot = reinterpret_cast<Byte *>( pSlabHeader ) + pSlabHeader->info.firstSlotOffset; k < pSlabHeader->info.slotCount;
                  ++k, pSlot += pSlabHeader->info.slotSize )
            {
                ftl::PushSinglyListNode<SlotHeader, &SlotHeader::pNext>( &m_freeList.pNext, reinterpret_cast<SlotHeader *>( pSlot ) );
            }
        }
        m_allocatedSlots = 0;
    }

protected:
    // @return number of slots segregated. -1 for too small slot for slot header.
    static int segregate_slab( ftl::Byte *slab,
                               const SlabInfo &slabInfo,
                               typename SlabHeader::NextElemPtrType &slatList,
                               typename SlotHeader::NextElemPtrType &slotFreeList )
    {
        new ( slab ) SlabHeader( slabInfo );
        std::size_t k = 0;
        for ( auto pSlot = slab + slabInfo.firstSlotOffset; k < slabInfo.slotCount; ++k, pSlot += slabInfo.slotSize )
        {
            ftl::PushSinglyListNode<SlotHeader, &SlotHeader::pNext>( &slotFreeList, reinterpret_cast<SlotHeader *>( pSlot ) );
        }
        ftl::PushSinglyListNode<SlabHeader, &SlabHeader::pNext>( &slatList, reinterpret_cast<SlabHeader *>( slab ) );
        return slabInfo.slotCount;
    }

    static bool populateSlabInfo( SlabInfo &slabInfo, const AllocRequest &r, std::size_t slabHeaderSize )
    {
        assert( r.slotSize > 0 );
        assert( r.minSlotsPerSlab > 0 );
        if ( r.slotSize == 0 || r.minSlotsPerSlab == 0 )
            return false;

        slabInfo.firstSlotOffset = ftl::align_up( r.slabAlignment + slabHeaderSize, r.slotAlignment ) - r.slabAlignment;
        slabInfo.slotSize = ftl::align_up( r.slotSize, r.slotAlignment );
        slabInfo.slabSize = slabInfo.firstSlotOffset + slabInfo.slotSize * r.minSlotsPerSlab;
        if ( r.alignupToSlabGranularity )
            slabInfo.slabSize = ftl::align_up( slabInfo.slabSize, typename SlabInfo::size_type( r.slabGranularity ) );

        slabInfo.slotCount = ( slabInfo.slabSize - slabInfo.firstSlotOffset ) / slabInfo.slotSize;
        return true;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief TypedPool object pool.
template<class T, std::size_t Alignment = sizeof( T ), bool MultiThreaded = true, bool ConstGrowthStrategy = true>
class TypedPool
{
protected:
    MemPool<MultiThreaded, ConstGrowthStrategy> m_memPool;

public:
    struct Deleter
    {
        TypedPool &alloc;
        Deleter( TypedPool &a ) : alloc( a )
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
        TypedPool &alloc;
        Allocator( TypedPool &a ) : alloc( a )
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

public:
    TypedPool() = default;
    TypedPool( std::size_t initialReserve ) : m_memPool( AllocRequest{sizeof( T ), initialReserve, Alignment} )
    {
    }
    bool init( std::size_t initialReserve )
    {
        return m_memPool.init( AllocRequest{sizeof( T ), initialReserve, Alignment} ) > 0;
    }

    T *allocate( size_t = 1 )
    {
        return reinterpret_cast<T *>( m_memPool.malloc() );
    }
    void deallocate( T *p, size_t = 1 )
    {
        m_memPool.free( p );
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
    void clear()
    {
        m_memPool.clear();
    }

    size_t allocated_size() const
    {
        return m_memPool.capacity() - m_memPool.free_size();
    }
    size_t free_size() const
    {
        return m_memPool.free_size();
    }
    Deleter to_deleter() const
    {
        return Deleter( *this );
    }
    Allocator to_allocator() const
    {
        return Allocator( *this );
    }
};

} // namespace ftl
