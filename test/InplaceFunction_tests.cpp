#include <ftl/unittest.h>

#include <ftl/inplace_function.h>
#include <functional>

ADD_TEST_CASE( FixedFunction_tests )
{
    //    {
    //        int *p = new int;

    //        int x = 30;
    //        std::cout << " in stack " << likely_stack_addr( &x ) << ", heap:" << likely_stack_addr( p ) << std::endl;
    //        delete p;
    //    }

    struct Functor
    {
        std::size_t val = 100;
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

        ~Functor()
        {
            std::cout << "functor destructing:" << val << std::endl;
        }
    };

    // std::function memory layout: { functor_storage: 16 bytes, pointer to object manager: 8 bytes, _M_invoker: 8 bytes }
    // effective size for functor is 16 bytes;
    using StdFunc = std::function<bool( int )>;
    static_assert( sizeof( StdFunc ) == 32, "Wrong size StdFunc!" );
    constexpr auto SIZE = 32u;
    using Func = ftl::InplaceFunction<bool( int ), SIZE, false, true>; // non-copy-constructible but move-constructible
    using MutFunc = ftl::MutableInplaceFunction<bool( int ), SIZE, false, true>;

    static_assert( sizeof( Func ) == SIZE, "Wrong size" );
    static_assert( sizeof( MutFunc ) == SIZE, "Wrong size" );

    SECTION( "StdFunction vs InplaceFunction" )
    {
        //-- test allocation
        //  std::function has size 32, effective size 16.
        bool isInStack;
        auto f0 = [this, &isInStack, y = std::size_t( 0 )]( int x ) mutable {
            jz::PrintStack( std::cout, 1 );

            isInStack = jz::is_likely_stack_addr( &y );
            std::cout << "functor is in stack:" << isInStack << ", test:" << x << std::endl;
            return true;
        };
        static_assert( sizeof( f0 ) == 24, "Wrong size" );
        StdFunc stdfunc = f0;
        stdfunc( 1 ); // functor is in stack
        REQUIRE( !isInStack );
        MutFunc mutFixfunc = f0;
        mutFixfunc( 1 ); // functor is in heap
        REQUIRE( isInStack );

        //-- test move construct/copy
        //    std::function only supports copy constructor.
        //        StdFunc stdfunc1 = Functor(); // compile error! std::function doesn't support move copy.
        //        stdfunc = Functor(); // compile error! std::function doesn't support move copy.
        mutFixfunc = Functor{205}; // InplaceFunction supports move copy.

        std::cout << "-- reassign functor --" << std::endl;
        //-- test destructor
        mutFixfunc = f0; // print destruct 205

        //-- static function
        //        struct my
        //        {
        //            static bool isodd( int x )
        //            {
        //                return x & 0x1;
        //            }
        //        };

        //        Func f1 = &my::isodd;
        //        f1.size();
        //        REQUIRE_EQ( f1.size(), 16u );
        //        REQUIRE( f1( 3 ) );
        //        stdfunc = &my::isodd;
        //        REQUIRE( stdfunc( 3 ) );
    }
    SECTION( "basic" )
    {
        int x = 0;
        auto f0 = [this, &x]( int inc ) mutable {
            x += inc;

            std::cout << "test:" << x << std::endl;
            return true;
        };
        static_assert( sizeof( f0 ) == 16, "Wrong size" );
        MutFunc mutF0 = f0;
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

    SECTION( "performance" )
    {
        constexpr auto SIZE = 32u;
        using Func = ftl::InplaceFunction<bool( int ), SIZE>;
    }
}
