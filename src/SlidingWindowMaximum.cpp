/**
  http://www.lintcode.com/en/problem/sliding-window-maximum/

Given an array of n integer with duplicate number, and a moving window(size k), move the window at each iteration from the start of the array, find the maximum number inside the window at each moving.

Example
For array [1, 2, 7, 7, 8], moving window size k = 3. return [7, 7, 8]
At first the window is at the start of the array like this
[|1, 2, 7| ,7, 8] , return the maximum 7;
then the window move one step forward.
[1, |2, 7 ,7|, 8], return the maximum 7;
then the window move one step forward again.
[1, 2, |7, 7, 8|], return the maximum 8;

Challenge
o(n) time and O(k) memory
 */
#include <vector>
#include <algorithm>
#include <iostream>
#include <list>
#include <set>
#include <memory>

using namespace std;

typedef std::vector<int> IntVector;

typedef std::multiset<int> IntSet;
typedef std::multiset<int>::iterator IntSetIter;
typedef std::list<IntSetIter> IntSetIterList;


class Solution {
public:
    /** naive implementation
     * @param nums: A list of integers.
     * @return: The maximum number inside the window at each moving.
     */
	IntVector maxSlidingWindow0(const vector<int> &nums, int k) {
        IntVector result;
        const int N = nums.size();
        if( N == 0 ) return result;
    	result.push_back(*std::max_element(nums.begin(), nums.begin() + std::min(N,k)));
        for(int i=k+1; i<= N; i+=1) {
        	int p = std::max(0, i-k);
        	int x = *std::max_element(&nums[0]+p, &nums[0]+i);
        	result.push_back(x);
        }
        return result;
    }
    /** Optimized algo
     * RBTree is used to keep sorted window and a list of pointer to element of RBTree.
     * So that element can be easily added/erased from tree&list and the back of tree is the largest element in window.
     * @param nums: A list of integers.
     * @return: The maximum number inside the window at each moving.
     */
	IntVector maxSlidingWindow(const vector<int> &nums, int k) {
        IntVector result;
        const int N = nums.size();
        if( N == 0 ) return result;
        int m = std::min(N,k);
    	result.push_back(*std::max_element(nums.begin(), nums.begin() + m));
    	if( N <= k ) {
    		return result;
    	}
    	IntSet sortedQ;
    	IntSetIterList window;
    	for(int i=0; i<m; ++i) {
    		window.push_back(sortedQ.insert(nums[i]));
    	}

        for(int i=k; i< N; i+=1) {
        	sortedQ.erase(window.front());
        	window.pop_front();
        	window.push_back(sortedQ.insert(nums[i]));
        	result.push_back(*sortedQ.rbegin());
//        	cout << i << ": " << nums[i] << " -> ";
//        	for(auto const &e: sortedQ) {
//        		cout << e << " ";
//        	}
//        	cout << endl;
        }
        return result;
    }
};

static void test() {
	Solution sln;
	if(1){
		IntVector v{1,2,9,7,8};
		auto r = sln.maxSlidingWindow(v, 3);
		for(auto const& x : r) {
			cout << x << " ";
		}
	}
	cout << endl;
	if(1){
		IntVector v{1,2,7,7,2,10,3,4,5};
		auto r = sln.maxSlidingWindow(v, 2);  // [2,7,7,7,10,10,4,5]
		for(auto const& x : r) {
			cout << x << " ";
		}
	}
}

int main()
{
	Solution sln;
	test();
	return 0;
}
