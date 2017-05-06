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

/*
 * KthSmallestSumFromSortedArrays.cpp

You have two sorted lists A and B, find the kth smallest sum.
Where sum is defined as a+b such that a belongs to list A and b belongs to list B.
For e.g, if A=[1,2,3] B=[4,5,6], the list of possible sums are [5,6,6,7,7,7,8,8,9],
and hence the 5th smallest sum is 7.

 */

namespace KthSmallestSumFromSortedArrays {
// take first k elements from array A and array B to generate k*k sums.
// push sums into a max heap.
// the one on top is the k-th smallest sume.
// Time complexity O(k*k*log(k)

typedef vector<int> IntVec;
int solve(const IntVec& A, const IntVec& B, int k) {
	IntVec q(k);
	int n = 0;
	int NA = A.size()>=k ? k: A.size();
	int NB = B.size() >= k? k: B.size();

	for(int i=0; i<NA; ++i) {
		for(int j=0; j<NB; ++j) {
			int s = A[i] + B[i];
			if( n == k ) {
				if( s < q[0] ) {
					pop_heap(q.begin(), q.end());
					q[n-1]= A[i] + B[j];
					push_heap(q.begin(), q.begin()+n);
				}
			} else {
				q[n++]= A[i] + B[j];
				push_heap(q.begin(), q.begin()+n);
			}
//			cout << n << "," << q << endl;
		}
	}
//	cout << q << endl;
	return q[0];
}

void test() {
	IntVec A{1,2,3}, B{4,5,6};
	cout << solve(A, B, 5) << endl; // 7
}

}

int main() {
	KthSmallestSumFromSortedArrays::test();
}

