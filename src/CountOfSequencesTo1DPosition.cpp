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

/*
 * CountOfSequencesTo1DPosition.cpp
 On a segment of line of length L, starting from position S, ending at E, giving a list of instructions [1, -1, 1... 1].
 1 means moving right, -1 for left.
 return the number of subsequence of instructions that move to E from S.
  */
namespace CountOfSequencesTo1DPosition {
typedef vector<int> IntVec;

int solve(const IntVec& v, int N, int S, int E) {
	IntVec dp(N);
	dp[S] = 1;
	for(int i=0; i<v.size(); ++i) {
		IntVec tdp(N);
		int x = v[i];
		for(int j=0; j<N; ++j) {
			tdp[j] = dp[j];
			if( j==0 ) {
				if( x<0 ) tdp[j] += dp[j+1];
			}else if( j==N-1) {
				if( x>0 ) tdp[j] += dp[j-1];
			}else{
				tdp[j] += dp[j-x];
			}
		}
		cout << i << ":" << tdp << endl;
		dp.swap(tdp);
	}
	return dp[E];
}
void test() {
	cout << "expected 1=" << solve({1}, 6, 1, 2) << endl;
	IntVec v{1,1, -1, 1, -1, 1};
	cout << "expected 7=" << solve(v, 6, 1, 2) << endl;
}
}

int main_CountOfSequencesTo1DPosition() {
	CountOfSequencesTo1DPosition::test();
}


