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
#include <assert.h>
#include <ftl/container_serialization.h>
using namespace std;
using namespace ftl::serialization;

namespace LargestValidParentheses {


class Solution {
public:

    // assume ending with "(..."
    void tryPushOne(deque<int> &q) {
        size_t n = q.size() -1;
        assert( q[n] < 0 );
        if( q[n] == -1 ) {
            if( n > 0 ) {  // "N("
                assert( q[n-1] > 0 );
                q.pop_back();
                ++q[n-1];
            }else{  // "("
                q[n] = 1;
            }
        } else {  // "...((("
            ++q[n];  // remove the last '('
            q.push_back(1);
        }
//        assert( q.last() > 0 );
    }
    int longestValidParentheses(string s) {
        deque<int> q;  // "-N, N, -N, N": -N is the number of single '(', N is numberof matches "()"
        size_t maxL = 0;
        for(size_t i=0; i<s.length(); ++i) {
            size_t n = q.size() - 1;
            if( s[i] == '(' ) {
                if( n < 0 || q[n] >= 0 ) {
                    q.push_back(-1);
                }else{
                    --q[n];
                }
            } else if( n>=0 ){   // ')'
                if( q[n] < 0 ) {  // ending with "...("
                    tryPushOne(q);
                }else{ // "...N"
                    if( n > 0 ) { // "...((N"
                        assert( q[n-1] < 0 );
                        int x = q[n];
                        q.pop_back();
                        tryPushOne(q);
                        n = q.size()-1;
                        q[n] += x;
                    }else{  // "N"
                        q.clear();
                    }
                }
                n = q.size()-1;
                if( q.size() > 0 && q[n] > maxL ) maxL = q[n];
            }
        }
        return maxL * 2;
    }
    int longestValidParentheses0(string s) {
        deque<int> q; // (((N((..."
        size_t maxL = 0;
        for(size_t i=0; i<s.length(); ++i) {
            if( s[i] == '(' ) {
                q.push_back(-1);
            } else {
                int n = q.size() - 1;
                int L = 1;
                while( n >= 0 && q[n] != -1 ) {
                    L += q[n];
                    q.pop_back();
                    --n;
                }
                if( n>=0 ) {
                    if( n > 0 && q[n-1] > 0 ) {
                        q.pop_back();
                        q[n-1] += L;
                        L = q[n-1];
                    }else
                        q[n] = L;

                    if( L > maxL ) maxL = L;
                }
            }
        }
        return maxL * 2;
    }
};


void test() {
    Solution sln;
    cout << sln.longestValidParentheses("()()") << endl; //4
    cout << sln.longestValidParentheses("(()(()") << endl; //2

}

}

//int main_LargestValidParentheses() {
int main() {

    LargestValidParentheses::test();
}
