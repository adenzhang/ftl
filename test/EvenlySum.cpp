/*
There are n groups of friends, and each group is numbered from 1 to n. The ith group contains ai people.

They live near a bus stop, and only a single bus operates on this route. An empty bus arrives at the bus stop and all the groups want to travel by the bus.

However, group of friends do not want to get separated. So they enter the bus only if the bus can carry the entire group.

Moreover, the groups do not want to change their relative positioning while travelling. In other words, group 3 cannot travel by bus, unless group 1 and group 2 have either (a) already traveled by the bus in the previous trip or (b) they are also sitting inside the bus at present.

You are given that a bus of size x can carry x people simultaneously.

Find the size x of the bus so that (1) the bus can transport all the groups and (2) every time when the bus starts from the bus station, there is no empty space in the bus (i.e. the total number of people present inside the bus is equal to x)?

Input Format
The first line contains an integer n (1≤n≤10e5). The second line contains n space-separated integers a1,a2,…,an (1≤ai≤10e4).

Output Format

Print all possible sizes of the bus in an increasing order.
*/

#include <cmath>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>
using namespace std;

typedef vector<size_t> IntVector;
typedef unordered_map<size_t, size_t> IntIntMap;

namespace EvenlySum {


    // assumption: v[i] > 0.
    struct Solution {
        IntVector v;
        Solution(const IntVector& nbs): v(nbs) {}

        void run() {
            //-- construct invert-accummulative-map - isv[i], i is the sum of values from v[0] to v[isv[i]].
            const size_t N = v.size();
            IntIntMap isv(N+1);
            isv[v[0]] = 0;
            size_t M = v[0];
            for(int i=1; i<N; ++i) {
                M += v[i];
                isv[M] = i;
            }

            // scan sums
            size_t s = 0;
            for(int i=0; i<N; ++i) {
                s += v[i];
                // scan to check whether the sum is a valid solution
                size_t k = s+s;
                while( k <=M && isv.find(k) != isv.end() )
                    k += s;
                if(k == M+s) {
                    // output
                    cout << s << " ";
                }
            }
            cout << endl;
        }
    };
    void test() {
        size_t av[]={1, 2, 1, 1, 1, 2, 1, 3};
        IntVector v(av, av+sizeof(av)/sizeof(size_t));
        Solution sln(v);
        sln.run();
    }
}

int main_EvenlySum() {

//    EvenlySum::test();
    int n = 0;
    cin >> n;
    IntVector v(n);
    for(int i=0; i<n; ++i) {
        int x;
        cin >> x;
        v[i] = x;
    }
    EvenlySum::Solution sln(v);
    sln.run();

    return 0;
}
