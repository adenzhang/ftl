#include <ftl/unittest.h>
#include <ftl/shared_object_pool.h>
#include <iostream>

namespace
{
struct A
{
    int id;
    int pad;
    A( int id = 0 ) : id( id )
    {
        std::cout << "A()" << id << std::endl;
    }
    A( const A &a ) : id( a.id )
    {
        std::cout << "A(A)" << id << std::endl;
    }
    ~A()
    {
        std::cout << "~A()" << id << std::endl;
    }
};
typedef ::std::shared_ptr<A> APtr;
} // namespace

ADD_TEST_FUNC( test_shared_object_pool )
{
    using namespace ftl;
    typedef shared_object_pool<A> SharedAPool;
    using FixedAPool = fixed_shared_pool<A, 2>;

    struct my
    {
        static APtr getObject()
        {
            auto pool = SharedAPool::create_me();
            static int count = 0;
            return pool->create_shared( ++count );
        }
        static APtr getObjectFromFixedPool()
        {
            auto pool = FixedAPool::create_me();
            static int count = 0;
            return pool->create_shared( ++count );
        }
    };
    //    SECTION("TestShared")
    //    {
    //        APtr a;
    //        {
    //            APtr b, c;
    //            a = my::getObject();
    //            cout << "Got A:" << a->id << endl;
    //            b = a;
    //            c = my::getObject();
    //            a = c;
    //        }

    //    }
    //    SECTION( "TestFixedShared" )
    {
        APtr a;
        {
            APtr b, c;
            a = my::getObjectFromFixedPool();
            b = my::getObjectFromFixedPool();
            c = my::getObjectFromFixedPool();
            REQUIRE( APtr() != a );
            REQUIRE( APtr() != b );
            REQUIRE( c );

            std::cout << "Got A:" << a->id << std::endl;
            b = a;
            c = my::getObject();
            a = c;

            //
            auto pool = FixedAPool::create_me();
            size_t count = 10;
            a = pool->create_shared( ++count );
            b = pool->create_shared( ++count );
            c = pool->create_shared( ++count );
            REQUIRE( !!b );
            REQUIRE( !c );
        }
    }
}
