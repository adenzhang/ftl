
#include <cmath>
#include <cstdio>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <limits>
#include <algorithm>
#include <deque>
#include <memory>
#include <unordered_map>

using namespace std;

class Solution {
public:
    // assumption: d > n;
    // output n = (n*10)%d
    // return n*10/d;
    static unsigned int multiply10ThenDiv(unsigned int& n, const unsigned int d) {
        const int ND = 10;
        unsigned int a = n*ND;
        if (a / ND != n) {  // overflow
            a = 0;
            unsigned int r = 0;
            unsigned int k = 0;
            do {
                while( a < d-n  && k< ND) {
                    a += n;
                    ++k;
                }
                if( d == a ) {
                    a -= d;
                    ++r;
                }else if( k != ND ) {
                    a = n - (d-a);
                    ++k;
                    ++r;
                }
            }while(k < ND);
            n = a;
            return r;
        }else{
            n = a%d;
            return a/d;
        }
    }

    string fractionToDecimal(int sN, int sD) {
        typedef unordered_map<int,int> IntIntMap;
        const int L = 128;
        int negative = sN==0 || sD == 0? 0: (sN<0 ^ sD<0);
        unsigned int N = sN<0? (-sN): sN;
        unsigned int D = sD<0? (-sD): sD;
        unsigned int d = N/D;
        char buf[L];
        sprintf(buf, negative?"-%u":"%u", d);

        string s;
        IntIntMap historyR;
        unsigned int r = N%D * (N<0?(-1):1);
        for(int k=0; r != 0; ++k ) {
            IntIntMap::iterator it = historyR.find(r);
            if( it == historyR.end() ) {
                historyR.insert(make_pair(r, k));
            }else{  // found repetition
                int j = it->second;
                s = s.substr(0, j) + "(" + s.substr(j, s.length()-j) + ")";
                break;
            }
            s += (char)('0'+(int) multiply10ThenDiv(r, D));
        }
        s = string(buf) + (s.empty() ? "" : "."+s);
        return s;
    }
};

static void test()
{
    Solution sln;
//    unsigned int x = 1000000000;
//    cout << Solution::multiply10ThenDiv(1000000000, 2147483648) << endl;
//    cout << sln.fractionToDecimal(0,-13) << endl;
//    cout << sln.fractionToDecimal(-1,13) << endl;
//    cout << sln.fractionToDecimal(-3, -1) << endl;
//    cout << sln.fractionToDecimal(-2147483648, -1) << endl;
//    cout << sln.fractionToDecimal(-2147483648, 1) << endl;
//    cout << sln.fractionToDecimal(-1, -2147483648) << endl;
    int a[] = {1,2,3};
    next_permutation(a, a+3);
}

int main_FactionToDecimal(int, const char **) {
    test();
    return 0;
}
