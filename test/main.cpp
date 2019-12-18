
//#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER
// define CATCH_CONFIG_RUNNER if main() is supplied
#include <ftl/catch_or_ignore.h>



int main( int argc, const char *argv[] )
{
    //========= RUN C++ TESTS ==================================

    return unittests_main( argc, argv );
    //    {
    //        Catch::Session session;

    //        iRet = session.applyCommandLine( argc, argv );

    //        if ( iRet == 0 )
    //        {
    //            session.configData().shouldDebugBreak = true;
    //            session.configData().showDurations = Catch::ShowDurations::Always;

    //            const auto start = std::chrono::steady_clock::now();
    //            iRet = session.run();
    //            const auto stop = std::chrono::steady_clock::now();
    //            auto duration = std::chrono::duration_cast<std::chrono::microseconds>( stop - start );
    //            std::printf( "\n===============================================================================\n"
    //                         "Tests took %ld us\n"
    //                         "===============================================================================\n\n",
    //                         duration.count() );
    //        }
    //    }

    //    return iRet;
}
