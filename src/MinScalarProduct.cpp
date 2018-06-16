#include <algorithm>
#include <ftl/container_serialization.h>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*
 *
Problem

You are given two vectors v1=(x1,x2,...,xn) and v2=(y1,y2,...,yn). The scalar product of these vectors is a single number, calculated as x1y1+x2y2+...+xnyn.

Suppose you are allowed to permute the coordinates of each vector as you wish. Choose two permutations such that the scalar product of your two new vectors is the smallest possible, and output that minimum scalar product.

Input

The first line of the input file contains integer number T - the number of test cases. For each test case, the first line contains integer number n. The next two lines contain n integers each, giving the coordinates of v1 and v2 respectively.
Output

For each test case, output a line

Case #X: Y
where X is the test case number, starting from 1, and Y is the minimum scalar product of all permutations of the two given vectors.
Limits

Small dataset

T = 1000
1 ≤ n ≤ 8
-1000 ≤ xi, yi ≤ 1000

Large dataset

T = 10
100 ≤ n ≤ 800
-100000 ≤ xi, yi ≤ 100000

Sample


Input

Output

2
3
1 3 -5
-2 4 1
5
1 2 3 4 5
1 0 1 0 1

Case #1: -25
Case #2: 6


 */
using namespace std;
//using namespace ftl::serialization;

struct MinScalarProduct {
    using IntVec = std::vector<int>;

    int solve(IntVec& v1, IntVec& v2)
    {
        std::sort(v1.begin(), v1.end());
        std::sort(v2.begin(), v2.end());
        //        cout << "sorted v1:" << v1 << endl;
        //        cout << "sorted v1:" << v1 << endl;
        auto it1 = v1.begin(), end1 = v1.end();
        auto it2 = v2.rbegin(), end2 = v2.rend();
        int res = 0;
        for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
            res += *it1 * (*it2);
        }
        return res;
    }

    void main()
    {
        size_t ncase;
        cin >> ncase;
        for (size_t k = 0; k < ncase; ++k) {
            size_t nElem;
            cin >> nElem;
            IntVec v1(nElem), v2(nElem);
            for (size_t i = 0; i < nElem; ++i)
                cin >> v1[i];
            for (size_t i = 0; i < nElem; ++i)
                cin >> v2[i];
            cout << "Case #" << k + 1 << ": " << solve(v1, v2) << endl;
        }
    }

    void run_test()
    {
        stringstream ssIn;
        ssIn << R"(2
              3
              1 3 -5
              -2 4 1
              5
              1 2 3 4 5
              1 0 1 0 1
              )";
        auto redirectin = ftl::make_scoped_stream_redirect(ssIn, std::cin);

        main();
    }
};

int main()
{
    MinScalarProduct sln;
    //    sln.main();
    sln.run_test();
}
