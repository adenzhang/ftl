
//#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER
// define CATCH_CONFIG_RUNNER if main() is supplied
#include <ftl/catch_or_ignore.h>

#include <algorithm>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <ftl/container_serialization.h>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>

using namespace std;
using namespace ftl::serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

struct A
{
    virtual void f()
    {
    }
};
struct B : virtual A
{
    virtual void f()
    {
    }
};
struct C : virtual A
{
    virtual void f()
    {
    }
};
struct D : B, C
{
    virtual void f()
    {
    }
};

class String
{
public:
    explicit String( char )
    {
    }
    String( const char * )
    {
    }

private:
    void operator=( const char * )
    {
    }
};
int main_app()
{

    cout << "v:" << sqrt( 5.9 ) << endl;
    cout << "m:" << sqrt( -4.5 ) << endl;
    return 0;
}

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
