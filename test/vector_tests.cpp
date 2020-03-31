#include <ftl/vector.h>
#include <ftl/ftl.h>
#include <algorithm>
using namespace ftl;

ADD_TEST_CASE( vector_tests )
{
    SECTION( "vector" )
    {
        std::vector<int> v{0, 1, 2, 3};
        std::remove_if( v.begin(), v.end(), []( const auto &x ) { return x & 0x01; } );
        auto p0 = v.begin(), p1 = std::next( p0 );
        v.erase( p0, p1 );
    }

    Array<char, 5> a = {'a', 'b', 'c'};

    REQUIRE( a.is_string == false );
    REQUIRE( a.using_inplace() == true );
    REQUIRE( a.capacity() == 5 );
    REQUIRE( a.inplace_capacity == 5, << a.inplace_capacity );
    REQUIRE( a.size() == 3 );

    //- test copy from Array to Vector
    Vector<char, 3> v( a );

    REQUIRE( v.is_string == false );
    REQUIRE( v.capacity() == 3 );
    REQUIRE( v.using_inplace() == true );
    REQUIRE( v.size() == 3 );

    v.push_back( 'd' ); // it's abc
    REQUIRE( v.size() == 4 );
    REQUIRE( v.capacity() == 6, "cap: " << v.capacity() ); // capacity doubled
    REQUIRE( v.using_inplace() == false ); // using new allocated memory

    //- copy from vector to string
    String ts( v ); // "abcd", using inplace buffer.
    REQUIRE( ts.is_string == true );
    REQUIRE( ts.using_inplace() == true );
    REQUIRE( ts.size() == 4, << ts.c_str() );
    REQUIRE( ts.capacity() == ts.inplace_capacity, << ts.capacity() << " != " << ts.inplace_capacity );
    REQUIRE( ts.using_inplace() == true );

    //- move from vector to string
    String s( std::move( v ) ); // move internal buffer from v to s.

    REQUIRE( s.is_string == true );
    REQUIRE( s.using_inplace() == false );
    REQUIRE( s.size() == 4 );
    REQUIRE( s.capacity() == 5, << s.c_str() << ", cap:" << s.capacity() << ", size:" << s.size() );
    REQUIRE( s.buffer_size() == 6 );

    // v is moved out
    REQUIRE( v.size() == 0 );
    REQUIRE( v.capacity() == v.inplace_capacity );
    REQUIRE( v.using_inplace() == true );

    //- sub, substr
    auto subs = s.substr( 1 );
    REQUIRE( subs.using_inplace() );
    REQUIRE( subs.size() == 3 );
    REQUIRE( subs == s.sub_view( 1 ) );
    REQUIRE( subs == make_view( "bcd" ), << subs.c_str() << " != bcd" );

    //- test shink_to_fit
    {
        CharCStr<3> s( "1" );
        REQUIRE( s.using_inplace() );
        s.shrink_to_fit();
        REQUIRE( s.size() == 1, "size:" << s.size() );
        REQUIRE( s.capacity() == 2 );

        s += "23";
        REQUIRE( s == "123", << s.c_str() );
        REQUIRE( s.size() == 3, "size:" << s.size() );
        REQUIRE( !s.using_inplace() );
        REQUIRE( s.buffer_size() == 6, << s.c_str() << " bufsize:" << s.buffer_size() );
        REQUIRE( s.capacity() == 5 );

        s.shrink_to_fit( 4 );
        REQUIRE( s == "123", << s.c_str() );
        REQUIRE( s.capacity() == 4, << s.c_str() );

        s += "45";
        REQUIRE( s.buffer_size() == 10, << s.c_str() << " bufsize:" << s.buffer_size() );

        REQUIRE( 1 == s.pop_back( s.size() - 1 ), << s.c_str() ); // keep 1 only
        REQUIRE( s.buffer_size() == 10 );
        s.shrink_to_fit();
        REQUIRE( s.buffer_size() == 3 );
    }

    //- test erase
    {
        Vector<int, 3> v( 5 );
        REQUIRE( v.size() == 5 && v.capacity() == 5 );
        std::iota( v.begin(), v.end(), 1 );
        v.erase( v.begin() + 1, v.begin() + 3 );

        REQUIRE( v == ( Vector<int, 3>{1, 4, 5} ) );
    }
}
