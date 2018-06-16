
/*

  Given n non-negative integers representing the histogram's bar height where the width of each bar is 1, find the area of largest rectangle in the histogram.

For example,
Given height = [2,1,5,6,2,3],
return 10.

*/
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
#include <string.h>

using namespace std;


class Solution {
public:
    typedef vector<int> IntVector;

    void popToUpperBound(IntVector& v, int bound) {
        while(v.size() > 1 && v.back() >= bound && v[v.size()-1] >= bound) {
            v.pop_back();
        }
    }

    int largestRectangleArea(IntVector &height) {
        IntVector iTroughs;
        iTroughs.reserve(height.size());
        iTroughs.push_back(height[0]);
        int area = 0;
        int asc = 0; // 1 for ascending; 0 for descending;
        for(int i=1; i<height.size(); ++i) {
            int h = height[i];
            if( h > height[i-1] ) {
                iTroughs.push_back(i);
                asc = 1;
            }else if ( h < height[i-1] || (h==height[i-1] && !asc) ) {
                popToUpperBound(iTroughs, h);
                int k = iTroughs.back();
                int a = (i-k) *height[k];
                // todo
                asc = 0;
            }else{
                // todo
            }
        }
    }
};
static void test() {

}

int main_LargestRect(int, const char **) {
    test();
    return 0;
}
