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

// slabs are linked into list. Each slab contains multiple slots which are can allocated by calling MemPool::malloc().
// slab 0: |SlabHeader|..|slot 1|slot 2|....|
//            |
// slab 1: |SlabHeader|..|slots ...|
// other slabs ....

template<bool IsAtomic>
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

// SlotHeader doesn't calculated as part of slot size.
template<bool IsAtomic>
struct SlotHeader
{
    using NextElemPtrType = std::conditional_t<IsAtomic, std::atomic<SlotHeader *>, SlotHeader *>;
    NextElemPtrType pNext{};
};

template<bool IsAtomic>
struct SlabInfo
{
    std::size_t firstSlotOffset, slotSize, slotCount;

    // @return number of slots segregated. -1 for too small slot for slot header.
    int Init( std::size_t slabAddr, std::size_t slabsize, std::size_t slotAlignment, std::size_t slotMinSize )
    {
        if ( slabsize < sizeof( SlabHeader<IsAtomic> ) )
            return -1;
        firstSlotOffset = ftl::align_up( slabAddr + sizeof( SlabHeader<IsAtomic> ), slotAlignment ) - slabAddr;
        if ( firstSlotOffset >= slabsize )
            return 0;
        slotSize = ftl::align_up( slotMinSize, slotAlignment );
        slotCount = ( slabsize - firstSlotOffset ) / slotSize;
        return slotCount;
    }
};

// @return number of slots segregated. -1 for too small slot for slot header.
template<bool IsAtomic>
int segregate_slab( ftl::Byte *slab,
                    std::size_t slabsize,
                    std::size_t slotMinSize,
                    std::size_t slotAlignment,
                    typename SlabHeader<IsAtomic>::NextElemPtrType &slatList,
                    typename SlotHeader<IsAtomic>::NextElemPtrType &slotFreeList )
{
    SlabInfo<IsAtomic> slabInfo;
    if ( slabInfo.Init( reinterpret_cast<std::size_t>( slab ), slabsize, slotAlignment, slotMinSize ) < 0 )
        return -1;
    new ( slab ) SlabHeader<IsAtomic>( slabsize );
    std::size_t k = 0;
    for ( auto pSlot = slab + slabInfo.firstSlotOffset; k < slabInfo.slotCount; ++k, pSlot += slabInfo.slotSize )
    {
        ftl::PushSinglyListNode( &slotFreeList, reinterpret_cast<SlotHeader<IsAtomic> *>( pSlot ), &SlotHeader<IsAtomic>::pNext );
    }
    ftl::PushSinglyListNode( &slatList, reinterpret_cast<SlabHeader<IsAtomic> *>( slab ), &SlabHeader<IsAtomic>::pNext );
    return slabInfo.slotCount;
}

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
struct MemPool
{
    SlotHeader<IsAtomic> m_freeList;
    SlabHeader<IsAtomic> m_slabList;

    AllocRequest m_defaultAllocReq;
    std::atomic<std::ptrdiff_t> m_totalSlots = 0, m_allocatedSlots = 0; // for tracking allocations.

    MemPool() = default;

    // @param slotSize size of each slot which is allocated by calling malloc().
    // @param nimSlotsPerSlab indicates min number of slots per slot.
    // @param slabGranuity usually multiple of page size (4096).
    MemPool( const AllocRequest &ar )
    {
        Init( ar );
    }

    int Init( const AllocRequest &ar )
    {
        m_defaultAllocReq = ar;
        return allocate_slab( ar );
    }

    // user may directly call allocate_slab to allocate a slab of slots.
    // @return number of slots allocated. -1 for error.
    int allocate_slab( const AllocRequest &r )
    {
        std::size_t firstSlotOffset = ftl::align_up( sizeof( SlabHeader<IsAtomic> ), r.slotAlignment );
        auto slotSize = ftl::align_up( r.slotSize, r.slotAlignment );
        std::size_t slabSize = firstSlotOffset + slotSize * r.minSlotsPerSlab;
        if ( r.alignupToSlabGranularity )
            slabSize = ftl::align_up( slabSize, r.slabGranularity );

        auto pSlab = ftl::sys_aligned_reserve_and_commit( slabSize, r.slabAlignment, r.slabGranularity );
        if ( !pSlab ) // failed allocatation.
            return -1;
        auto ret = segregate_slab<IsAtomic>( pSlab, slabSize, r.slotSize, r.slotAlignment, m_slabList.pNext, m_freeList.pNext );
        assert( ret > 0 );
        if ( ret > 0 )
            m_totalSlots.fetch_add( std::ptrdiff_t( ret ) );
        return ret;
    }

    ~MemPool()
    {
        assert( free_size() == capacity() );
        while ( auto p = ftl::PopSinglyListNode( &m_slabList.pNext, &SlabHeader<IsAtomic>::pNext ) )
        {
            ftl::sys_release( p, p->m_slabSize );
        }
    }

    // allocate a slot.
    void *malloc()
    {
        auto p = ftl::PopSinglyListNode( &m_freeList.pNext, &SlotHeader<IsAtomic>::pNext );
        if ( !p )
        {
            assert( m_defaultAllocReq.slotSize );
            if ( !m_defaultAllocReq.slotSize )
                return nullptr;

            // slow path
            allocate_slab( m_defaultAllocReq );
        }
        if ( ( p = ftl::PopSinglyListNode( &m_freeList.pNext, &SlotHeader<IsAtomic>::pNext ) ) )
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
        ftl::PushSinglyListNode( &m_freeList.pNext, reinterpret_cast<SlotHeader<IsAtomic> *>( p ), &SlotHeader<IsAtomic>::pNext );
    }

    std::size_t capacity() const
    {
        return m_totalSlots.load();
    }

    std::size_t free_size() const
    {
        return m_totalSlots.load() - m_allocatedSlots.load();
    }
};

} // namespace ftl
