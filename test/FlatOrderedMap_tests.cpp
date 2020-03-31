#include <ftl/unittest.h>
#include <ftl/flat_ordered_map.h>

ADD_TEST_CASE( FlatOrderedMap_tests )
{
    SECTION( "int map" )
    {
        ftl::FlatOrderedMap<int, int> m = {{2, 3}, {1, 4}};
        REQUIRE( m.front().first == 1 && m.size() == 2 );
        m[-1] = 20;
        REQUIRE( m.front().first == -1 && m.size() == 3 );
        m.erase( -1 );
        REQUIRE( m.front().first == 1 && m.size() == 2 );
    }
    SECTION( "string map" )
    {
        ftl::FlatOrderedSet<std::string> s{"cd", "ab"};
        REQUIRE( s.front() == "ab" );
        s += "aa";

        REQUIRE( s.front() == "aa" );
        s.erase( "aa" );
        REQUIRE( s.front() == "ab" );
    }
}
