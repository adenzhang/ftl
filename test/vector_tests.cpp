#include <ftl/vector.h>
#include <ftl/ftl.h>

using namespace ftl;
ADD_TEST_FUNC( vector_tests )
{
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
    REQUIRE( v.capacity() == 6 ); // capacity doubled
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
    REQUIRE( s.capacity() == 6, << s.c_str() << ", cap:" << s.capacity() << ", size:" << s.size() );

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
}
