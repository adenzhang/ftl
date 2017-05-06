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
#include "ContainerSerialization.h"
using namespace std;
using namespace serialization;

/**
 Objec­tive: Given a rod of length n inches and a table of prices pi, i=1,2,…,n,
 write an algo­rithm to find the max­i­mum rev­enue rn obtain­able by cut­ting up the rod and sell­ing the pieces.

Given: Rod lengths are inte­gers and For i=1,2,…,n we know the price pi of a rod of length i inches

Exam­ple:

Length	1	2	3	4	5	6	7	8	9	10
Price	1	5	8	9	10	17	17	20	24	30

for rod of length: 4
Ways to sell :
•	selling length : 4  ( no cuts) , Price: 9
•	selling length : 1,1,1,1  ( 3 cuts) , Price: 4
•	selling length : 1,1,2  ( 2 cuts) , Price: 7
•	selling length : 2,2  ( 1 cut) , Price: 10
•	selling length : 3, 1  ( 1 cut) , Price: 9

 *
 */

namespace CutRod{
typedef vector<int> IntVector;

int cut_rod(int length, std::vector<int>& prices, std::vector<int>lengths) {
	IntVector dp(length+1);  // dp[i]: best price selling a rod of length i
	for(int i=1, p = prices[0], x = lengths[0]; i <= length; ++i) {
		dp[i] = (i/x) * p;
	}
	for(int i=1; i < lengths.size(); ++i) {
		IntVector tdp(length+1);
		int x = lengths[i];
		int p = prices[i];
		for(int j=1; j<= length; ++j) {
			int m = (j/x) * p; // max price selling length-j rod
			for(int k=0; j-k*x>0; ++k) {
				int y = j-k*x;
				if( dp[y] > m ) {
					m = dp[y];
				}
			}
			tdp[j] = m;
		}
		dp.swap(tdp);
	}
	return dp[length];
}

void test() {
	IntVector lengths{1,2,3,4,5,6,7,8,9,10};
	IntVector prices{1,	5,	8,	9,	10,	17,	17,	20,	24,30};
	cout << cut_rod(4, prices, lengths) << endl; // 10

}

}

int main() {
	CutRod::test();
}
