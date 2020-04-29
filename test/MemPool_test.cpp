#include <ftl/unittest.h>
#include <ftl/mem_pool.h>

using namespace ftl;

ADD_TEST_CASE( MemPool_tests )
{
    SECTION( "basic" )
    {

        MemPool<false> pool( AllocRequest{100, 500} );
        REQUIRE_EQ( pool.capacity(), 500 );
        REQUIRE_EQ( pool.free_size(), 500 );
        auto p = pool.malloc();
        REQUIRE( p );
        REQUIRE_EQ( pool.free_size(), 499 );
        pool.free( p );
        REQUIRE_EQ( pool.free_size(), 500 );
    }
}
