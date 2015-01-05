/*
Say you have an array for which the ith element is the price of a given stock on day i.

Design an algorithm to find the maximum profit. You may complete at most two transactions.

Note:
You may not engage in multiple transactions at the same time (ie, you must sell the stock before you buy again).
*/

#include <iostream>
#include <vector>
#include <initializer_list>
#include <assert.h>

using namespace std;

class Solution {
public:
    // long and short are allowed. so maxProfit = HighestPrice - LowestPrice
    int maxProfit(const vector<int> &prices) {
        const int N = prices.size();
        if( N < 2 ) return 0;
        int maxP = prices[0], minP = prices[0];
        for(int i=1; i<N; ++i) {
            if(prices[i] > maxP) maxP = prices[i];
            if(prices[i] < minP) minP = prices[i];
        }
        return maxP - minP;
    }

    // buy first then sell
    int maxProfit0(const vector<int> &prices) {
        const int N = prices.size();
        if( N < 2 ) return 0;

        const int *p = &prices[1];
        int maxPrf = 0;
        int lowestPrice = prices[0];
        const int NTRANS = 2;  // number of transactions
        vector<int> iBuyAt(2, lowestPrice);
        vector<int> iSellAt(2, 0);
        int iMaxPrf,iLowPrice;
        for(int k=0; k<NTRANS; ++k) {
            for(int i=1; i<N; ++i, ++p) {
                int currPrf = *p - lowestPrice;
                if( currPrf >= 0 ) {
                    if( currPrf > maxPrf ) {
                        maxPrf = currPrf;
                        iMaxPrf = i;
                    }
                }else{
                    lowestPrice = *p;
                    iLowPrice = i;
                }
            }
        }
        return maxPrf;
    }
};


#define RUNTEST sln.maxProfit
#define ASSERT assert
static void test() {
    Solution sln;
    ASSERT( 5 == RUNTEST({3,2,4,5,1,2,0}) );
}

int main_maxProfit() {
    test();
    cout<< "The end." << endl;
    return 0;
}
