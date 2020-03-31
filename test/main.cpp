#include <ftl/unittest.h>
#include <ftl/ftl.h>

class Solution
{
public:
    // ways of i steps to reach j: D[i, j] = D[i-1, j-1] + D[i-1,j] + D[i-1,j+1], where D[M, N]
    int numWays( int M, int N )
    {
        static constexpr size_t MOD = 100000000 + 7;
        std::vector<size_t> D( N, 0 ), tD( N, 0 ); // N represents infinity.
        D[0] = 1; // i=0, 0 steps
        for ( size_t i = 1; i <= M; ++i )
        {
            for ( size_t j = 0; j < N; ++j )
            {
                tD[j] = D[j];
                if ( j > 0 )
                    tD[j] += D[j - 1];
                if ( j < N - 1 )
                    tD[j] += D[j + 1];
                tD[j] %= MOD;
            }
            D.swap( tD );
        }
        return D[0];
    }
};
ADD_TEST_CASE( test_1269 )
{
    Solution sln;
    REQUIRE_EQ( 127784505, sln.numWays( 27, 7 ) );
}
UNITTEST_MAIN
