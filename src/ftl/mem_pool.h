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
};

/// \brief MemPool preallocates slots. User calls MemPool::malloc() to allocate a slot.
/// Multiple slots are allocated in a slab. Multiple slabs are linked into a list.
/// The Memory pool is thread-safe only if IsAtomic.
template<bool IsAtomic>
class MemPool
{
protected:
    // slabs are linked into list. Each slab contains multiple slots which are can allocated by calling MemPool::malloc().
    // slab 0: |SlabHeader|..|slot 1|slot 2|....|
    //            |
    // slab 1: |SlabHeader|..|slots ...|
    // other slabs ....
    struct SlabHeader
    {
        using NextElemPtrType = std::conditional_t<IsAtomic, std::atomic<SlabHeader *>, SlabHeader *>;
        NextElemPtrType pNext{};
        std::size_t m_slabSize = 0;

        SlabHeader() = default;
        SlabHeader( std::size_t n ) : m_slabSize( n )
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
    std::atomic<std::ptrdiff_t> m_totalSlots = 0, m_allocatedSlots = 0; // for tracking allocations.

    struct SlabInfo
    {
        std::size_t slabSize, firstSlotOffset, slotSize, slotCount;
    };

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

        auto pSlab = ftl::sys_aligned_reserve_and_commit( slabInfo.slabSize, r.slabAlignment, r.slabGranularity );
        if ( !pSlab ) // failed allocatation.
            return -1;
        auto ret = segregate_slab( pSlab, slabInfo, m_slabList.pNext, m_freeList.pNext );
        assert( ret > 0 );
        if ( ret > 0 )
            m_totalSlots.fetch_add( std::ptrdiff_t( ret ) );
        return ret;
    }

    ~MemPool()
    {
        assert( free_size() == capacity() );
        while ( auto p = ftl::PopSinglyListNode( &m_slabList.pNext, &SlabHeader::pNext ) )
        {
            ftl::sys_release( p, p->m_slabSize );
        }
    }

    // allocate a slot.
    void *malloc()
    {
        auto p = ftl::PopSinglyListNode( &m_freeList.pNext, &SlotHeader::pNext );
        if ( !p )
        {
            assert( m_defaultAllocReq.slotSize );
            if ( !m_defaultAllocReq.slotSize )
                return nullptr;

            // slow path
            allocate_slab( m_defaultAllocReq );
        }
        if ( ( p = ftl::PopSinglyListNode( &m_freeList.pNext, &SlotHeader::pNext ) ) )
        {
            auto res = m_allocatedSlots.fetch_add( 1 );
            assert( res >= 0 && res + 1 <= m_totalSlots.load() );
            return p;
        }
        return nullptr;
    }

    // free an allocated slot.
    void free( void *p )
    {
        auto res = m_allocatedSlots.fetch_sub( 1 );
        assert( res > 0 && res <= m_totalSlots.load() );
        ftl::PushSinglyListNode( &m_freeList.pNext, reinterpret_cast<SlotHeader *>( p ), &SlotHeader::pNext );
    }

    std::size_t capacity() const
    {
        return m_totalSlots.load();
    }

    std::size_t free_size() const
    {
        return m_totalSlots.load() - m_allocatedSlots.load();
    }

protected:
    // @return number of slots segregated. -1 for too small slot for slot header.
    static int segregate_slab( ftl::Byte *slab,
                               const SlabInfo &slabInfo,
                               typename SlabHeader::NextElemPtrType &slatList,
                               typename SlotHeader::NextElemPtrType &slotFreeList )
    {
        new ( slab ) SlabHeader( slabInfo.slabSize );
        std::size_t k = 0;
        for ( auto pSlot = slab + slabInfo.firstSlotOffset; k < slabInfo.slotCount; ++k, pSlot += slabInfo.slotSize )
        {
            ftl::PushSinglyListNode( &slotFreeList, reinterpret_cast<SlotHeader *>( pSlot ), &SlotHeader::pNext );
        }
        ftl::PushSinglyListNode( &slatList, reinterpret_cast<SlabHeader *>( slab ), &SlabHeader::pNext );
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
            slabInfo.slabSize = ftl::align_up( slabInfo.slabSize, r.slabGranularity );

        slabInfo.slotCount = ( slabInfo.slabSize - slabInfo.firstSlotOffset ) / slabInfo.slotSize;
        return true;
    }
};

} // namespace ftl
