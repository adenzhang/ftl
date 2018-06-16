#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cmath>
#include <ctime>
#include <climits>
#include <deque>
#include <queue>
#include <stack>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ftl/container_serialization.h>
using namespace std;
using namespace ftl::serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

namespace array_split_max_diff {

int solution_sqrt(int A, int B) {
	// return number of positive integer x that A << x*x << B.
    // write your code in C++14 (g++ 6.2.0)
    if( B < 0 ) return 0;
    if( A<0 ) A = 0;
    int s = int(sqrt(A)), e = int(sqrt(B));
    if( s*s < A ) ++s;
    if( (e+1)*(e+1) <= B ) ++e;
    return e-s+1;
}

int array_split_max_diff(vector<int> &A) {
	// split array into 2 non-empty parts S1:{A[0], ..., A[K]} and S2: {S[k+1], ..., S[N-1]},
	// so that the difference abs(max(S1) - max(S2)) is maximum.
	//  return the maximum diff.
    // write your code in C++14 (g++ 6.2.0)
    typedef vector<int> IntVec;
    int N = A.size();
    if( N == 0 ) return -1;
    IntVec mFor(N), mBack(N);
    int mx = A[0];
    for(int i=0; i<N; ++i) {
        mFor[i] = mx = max(A[i], mx);
    }
    mx = A[N-1];
    mBack[N] = 0;
    for(int i=N-1; i>=0; --i) {
        mBack[i] = mx = max(A[i], mx);
    }
    mx = -1;
    for(int i=0; i<N-1; ++i) {
        int x = mFor[i] - mBack[i+1];
        if( x < 0 ) x = -x;
        if( x > mx ) {
            mx = x;
        }
    }
    return mx;
}

}

int main_array_split_max_diff() {
}

