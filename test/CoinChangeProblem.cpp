/*
 * CoinChangeProblem.cpp
How many different ways can you make change for an amount, given a list of coins? In this problem, your code will need to efficiently compute the answer.

Task

Write a program that, given

The amount  to make change for and the number of types  of infinitely available coins
A list of  coins -
Prints out how many different ways you can make change from the coins to STDOUT.

The problem can be formally stated:

Given a value , if we want to make change for  cents, and we have infinite supply of each of  valued coins, how many ways can we make the change? The order of coins doesn’t matter.

Solving the overlapping subproblems using dynamic programming

You can solve this problem recursively, but not all the tests will pass unless you optimise your solution to eliminate the overlapping subproblems using a dynamic programming solution

Or more specifically;

If you can think of a way to store the checked solutions, then this store can be used to avoid checking the same solution again and again.
Input Format

First line will contain 2 integer N and M respectively.
Second line contain M integer that represent list of distinct coins that are available in infinite amount.
 */

#include <ftl/container_serialization.h>
#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <stack>
#include <sstream>

using namespace std;
using namespace ftl::serialization;

namespace CoinChangeProblem {
typedef vector<int> IntVec;
int solve(const IntVec& C, int N) {
    if( N < 0) return 0;
    if( C.size() == 0) return 0;
    IntVec dl(N+1);
    for(int i=1, x = C[0]; i<=N; ++i) {
        dl[i] = i >= x && i%x == 0 ? 1 :0;
    }
//    cout << "i:" << C[0] << ", dl:" << dl << endl;
    for(int i=1; i< C.size(); ++i) {
        IntVec cdl(N+1);
        for(int j=1; j <= N; ++j) {
            int& ndp = cdl[j];
            ndp = j > C[i] && j%C[i] == 0 ? 1 : 0;
            for(int k=0; k*C[i] < j; ++k)
                ndp += dl[j-k*C[i]];
        }

        dl = cdl;
//        cout << "i:" << C[i] << ", dl:" << dl << endl;
    }
    return dl[N];
}
}
int main_CoinChangeProblem() {
    using namespace CoinChangeProblem;
    cout << "ans:" << solve({1,2,3}, 4) << endl; // 4
    cout << "ans:" << solve({2, 5,3,6}, 10) << endl; // 5
    cout << "ans:" << solve({12}, 10) << endl; // 5
    return 0;
}


