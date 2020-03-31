#pragma once
#include <ftl/alloc_common.h>

namespace ftl
{

// The cache line size is 64 bytes (on modern intel x86_64 processors), but we use 128 bytes to avoid false sharing because the
// prefetcher may read two cache lines.  See section 2.1.5.4 of the intel manual:
// http://www.intel.com/content/dam/doc/manual/64-ia-32-architectures-optimization-manual.pdf
// This is also the value intel uses in TBB to avoid false sharing:
// https://www.threadingbuildingblocks.org/docs/help/reference/memory_allocation/cache_aligned_allocator_cls.htm
// FALSE_SHARING_SIZE 128
template<class T, class GrowthPolicy = DoubleAccumulatedGrowthPolicy, size_t Align = 128>
class ChunkAllocator : protected GrowthPolicy
{
    // memory layout : ChunckInfo| slot1 | slot2 | ... |
    // size of ChunckInfo is same as size of slot.
    struct ChunkInfo;

    struct Slot
    {
        // unsigned m_index;
        T m_data;

        T &getData()
        {
            return m_data;
        }
    };

    struct ChunkInfo
    {
        ChunkInfo *m_pNext = nullptr;
        unsigned m_cap = 0; // total slots
        unsigned m_size = 0; // used slots

        void init( unsigned cap )
        {
            m_cap = cap;
            m_size = 0;
            m_pNext = nullptr;
        }
        void clear()
        {
            m_size = 0;
        }
        // return updated size
        unsigned incSize()
        {
            return m_cap == m_size ? 0 : ++m_size;
        }

        bool full() const
        {
            return m_cap == m_size;
        }

        void addNext( ChunkInfo *p )
        {
            m_pNext = p;
        }
    };
    static constexpr size_t ChunkInfoSize = align_up( sizeof( ChunkInfo ), Align );
    static constexpr size_t SlotSize = align_up( sizeof( Slot ), Align );
    static constexpr bool IsPOD = std::is_pod<T>::value;
    static constexpr size_t BoundarySize = 1 << 12; // 4k page size

    static Slot &getSlot( ChunkInfo &info, size_t k )
    {
        return *reinterpret_cast<Slot *>( reinterpret_cast<char *>( &info ) + ChunkInfoSize + SlotSize * k );
    }
    // static ChunkInfo& getChunkInfo(Slot& slot){
    //     return *static_cast<ChunkInfo*>( reinterpret_cast<char*>(&info)  - slot.m_index * SlotSize - ChunkInfoSize);
    // }

    static Slot *allocate_slot( ChunkInfo &info )
    {
        return info.incSize() ? &getSlot( info, info.m_size - 1 ) : nullptr;
    }

    static ChunkInfo *allocate_chunk( size_t minSlots )
    {
        assert( minSlots );
        size_t nbytes = ChunkInfoSize + SlotSize * minSlots;
        nbytes = align_up( nbytes, BoundarySize );
        auto pChunk = static_cast<ChunkInfo *>( std::aligned_alloc( Align, nbytes ) );
        if ( pChunk )
        {
            pChunk->init( ( nbytes - ChunkInfoSize ) / SlotSize );
        }
        // std::cout << "alloc addr: " << pChunk << ", slots:" << pChunk->m_cap << ", bytes:" << nbytes << std::endl;
        assert( pChunk );
        return pChunk;
    }

public:
    ChunkAllocator( size_t nReserve = 0, size_t growthParam = 0 ) : GrowthPolicy( growthParam ? growthParam : nReserve )
    {
        if ( nReserve > 0 )
            add_chunk();
    }

    // copy constructor only copies parameters, not memory.
    ChunkAllocator( const ChunkAllocator &a ) : ChunkAllocator( a.m_totalSlots, a.get_grow_value() )
    {
    }
    ChunkAllocator( ChunkAllocator &&a ) : GrowthPolicy( a.get_grow_value() ), m_totalSlots( a.m_totalSlots ), m_pChunkList( a.m_pChunkList )
    {
        a.m_pChunkList = nullptr;
    }
    ~ChunkAllocator()
    {
        destroy();
    }

    T *allocate( size_t = 1 )
    {
        if ( !m_pChunkList || m_pChunkList->full() )
            add_chunk();
        assert( m_pChunkList );
        assert( !m_pChunkList->full() );
        return &allocate_slot( *m_pChunkList )->getData();
    }

    /// @brief recycle, mark all allocated slot un-allocated.
    void clear()
    {
        for ( auto pChunk = m_pChunkList; pChunk; pChunk = pChunk->m_pNext )
            pChunk->clear();
    }
    // destroy all, but no deallocate provided.
    void destroy()
    {
        while ( m_pChunkList )
        {
            auto p = m_pChunkList;
            m_pChunkList = m_pChunkList->m_pNext;
            // std::cout << "free addr:" << p << ", slots:" << p->m_cap << std::endl;
            std::free( p );
        }
        m_totalSlots = 0;
    }

protected:
    void add_chunk()
    {
        auto pChunk = allocate_chunk( GrowthPolicy::grow_to( m_totalSlots ) );
        pChunk->addNext( m_pChunkList );
        m_pChunkList = pChunk;
        m_totalSlots += m_pChunkList->m_cap;
        GrowthPolicy::get_grow_value() = m_pChunkList->m_cap;
    }
    size_t m_totalSlots = 0;
    ChunkInfo *m_pChunkList = nullptr;
};


} // namespace ftl
