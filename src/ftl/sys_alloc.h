#pragma once
#include <cstdint>
#include <ftl/alloc_common.h>
#include <sys/mman.h>
//#include <hugetlbfs.h>
#include <cassert>

namespace ftl
{

namespace internal
{

    inline Byte *mmap_impl( std::size_t uiLen, int iProt, int iFlags )
    {
        void *const addr = mmap( nullptr, uiLen, iProt, iFlags, -1, 0 );

        if ( addr == reinterpret_cast<void *>( -1 ) )
            return nullptr;

        return reinterpret_cast<Byte *>( addr );
    }

    inline Byte *mmap_aligned_impl( std::size_t uiLen, std::size_t uiAlignment, std::size_t uiGranularity, int iProt, int iFlags )
    {
        assert( ftl::is_pow2( uiAlignment ) );
        assert( ftl::is_pow2( uiGranularity ) );

        if ( uiAlignment < uiGranularity )
        {
            return mmap_impl( uiLen, iProt, iFlags );
        }

        const std::size_t uiAllocLen = uiLen + uiAlignment - uiGranularity;
        assert( uiAllocLen >= uiLen );

        void *const addr = mmap( nullptr, uiAllocLen, iProt, iFlags, -1, 0 );

        if ( addr == reinterpret_cast<void *>( -1 ) )
            return nullptr;

        Byte *const byteAddr = reinterpret_cast<Byte *>( addr );

        const std::size_t uiUnmapPrefixLen =
                static_cast<std::size_t>( ftl::align_up( std::size_t( byteAddr ), uiAlignment ) - std::size_t( byteAddr ) );

        const std::size_t uiUnmapSuffixLen = uiAllocLen - uiUnmapPrefixLen - uiLen;

        Byte *const alignedAddr = byteAddr + uiUnmapPrefixLen;

        if ( uiUnmapPrefixLen != 0 )
        {
            auto res = munmap( addr, uiUnmapPrefixLen );
            assert( res == 0 );
        }

        if ( uiUnmapSuffixLen != 0 )
        {
            auto res = munmap( alignedAddr + uiLen, uiUnmapSuffixLen );
            assert( res == 0 );
        }

        return alignedAddr;
    }

} // namespace internal

inline bool sys_lock( const Byte *p, std::size_t uiLen )
{
    return 0 == mlock( p, uiLen );
}

inline bool sys_unlock( const Byte *p, std::size_t uiLen )
{
    return 0 == munlock( p, uiLen );
}

inline Byte *sys_aligned_reserve_and_commit( std::size_t uiLen, std::size_t uiAlignment, std::size_t uiGranularity, bool populatePageTable = true )
{
    assert( is_pow2( uiAlignment ) );

    int iFlags = MAP_ANONYMOUS | MAP_PRIVATE;
    //    if ( pageType == PageType::Huge )
    //    {
    //        iFlags |= MAP_HUGETLB;
    //    }

    if ( populatePageTable )
    {
        iFlags |= MAP_POPULATE;
    }

    return internal::mmap_aligned_impl( uiLen, uiAlignment, uiGranularity, PROT_READ | PROT_WRITE, iFlags );
}

inline bool sys_release( void *p, std::size_t uiLen )
{
    return 0 == munmap( p, uiLen );
}


} // namespace ftl
