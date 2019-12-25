#include <cstdlib>
#include <ftl/unittest.h>
#include <ftl/out_printer.h>
#include <signal.h>
#include <thread>
#include <try_catch/try_catch.h>
#include <unistd.h>
using namespace ftl;

int read_and_execute_command( size_t n, bool bcrash = false )
{
    if ( bcrash )
    {
        outPrinter.println( "will crach" );
        int *p = nullptr;
        *p = 234;
    }
    else
    {
        int x = 3;
        return x + n;
    }
    return 0;
}
void test_try( size_t N, bool bcrash = false )
{
    auto tsStart = std::chrono::steady_clock::now();
    for ( int i = 0; i < N; ++i )
    {
        //        sleep(1);
        //        outPrinter.println(i, " will setjmp errno:", errno);
        FTL_TRY1
        {
            read_and_execute_command( i, bcrash );
        }
        FTL_CATCH1
        {
            outPrinter.println( " recovered from interuption!" );
        }
        FTL_TRY_END1
    }
    auto tsStop = std::chrono::steady_clock::now();
    std::cout << "- processing time(ns): " << ( tsStop - tsStart ).count() << ", latency(ns):" << ( tsStop - tsStart ).count() / N << std::endl;
}

ADD_TEST_FUNC( try_catch_tests )
{
    FTL_TRY_INSTALL1();
    size_t N = 10000000;
    test_try( 1, true );
    test_try( 1, false );
    std::thread th1( test_try, N, false );
    //    std::thread th2(test_try);
    th1.join();
    //    th2.join();
}
