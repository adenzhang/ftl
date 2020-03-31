#include <ftl/unittest.h>

#include <ftl/functional.h>


ADD_TEST_CASE( FixedFunction_tests )
{
    struct Functor
    {
        int val = 100;
        Functor() = default;
        Functor( const Functor & ) = delete;
        Functor( Functor && ) = default;

        // returns mutable or not
        bool operator()( int x )
        {
            val += x;
            std::cout << "mut test:" << val << std::endl;
            return true;
        }
        bool operator()( int x ) const
        {
            std::cout << "immut test:" << x << std::endl;
            return false;
        }
    };

    using Func = ftl::FixedFunction<bool( int ), 16, false, true>; // non-copy-constructible but move-constructible
    using MutFunc = ftl::MutableFixedFunction<bool( int ), 16, false, true>;
    int x = 0;
    MutFunc f0 = [this, &x]( int inc ) mutable {
        x += inc;
        std::cout << "test:" << x << std::endl;
        return true;
    };
    Func f1;
    Func f2 = Functor();
    f1 = f2;
    REQUIRE_EQ( false, f1( 3 ) ); // calls immutable
    REQUIRE_EQ( false, f2( 2 ) ); // calls immutable
    MutFunc f3 = std::move( f1 ); // immutable function is copied to mutable function.
    REQUIRE_EQ( false, f3( 10 ) ); // calls immutable

    f3 = Functor();
    REQUIRE_EQ( true, f3( 20 ) ); // calls mutable
}
