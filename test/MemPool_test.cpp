#include <ftl/unittest.h>
#include <ftl/mem_pool.h>

using namespace ftl;

ADD_TEST_CASE( MemPool_tests )
{
    SECTION( "basic" )
    {

        MemPool<true> pool( AllocRequest{100, 500} );
        REQUIRE_EQ( pool.capacity(), 500 );
        REQUIRE_EQ( pool.free_size(), 500 );
        auto p = pool.malloc();
        REQUIRE( p );
        REQUIRE_EQ( pool.free_size(), 499 );
        pool.free( p );
        REQUIRE_EQ( pool.free_size(), 500 );
        pool.clear();
    }

    SECTION( "auto_alloc_slab" )
    {
        const std::size_t cap = 2;
        std::vector<void *> slots( cap );
        MemPool<false> pool( AllocRequest{100, cap} );
        REQUIRE_EQ( pool.capacity(), cap );

        for ( auto &pslot : slots )
        {
            pslot = pool.malloc();
            REQUIRE( pslot );
        }
        REQUIRE_EQ( 0, pool.free_size() );
        auto p = pool.malloc();
        REQUIRE( p );
        REQUIRE_EQ( pool.capacity(), 2 * cap );
        pool.clear();
    }
}
