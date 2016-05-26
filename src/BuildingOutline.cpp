/**
http://www.lintcode.com/en/problem/building-outline/

Given N buildings in a x-axis，each building is a rectangle and can be represented by a triple (start, end, height)，where start is the start position on x-axis, end is the end position on x-axis and height is the height of the building. Buildings may overlap if you see them from far away，find the outline of them。

An outline can be represented by a triple, (start, end, height), where start is the start position on x-axis of the outline, end is the end position on x-axis and height is the height of the outline.

Example
Given 3 buildings：

[
  [1, 3, 3],
  [2, 4, 4],
  [5, 6, 1]
]
The outlines are：

[
  [1, 2, 3],
  [2, 4, 4],
  [5, 6, 1]
]
Note
Please merge the adjacent outlines if they have the same height and make sure different outlines cant overlap on x-axis.

*/
#include <vector>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

#include "ContainerSerialization.h"

using namespace std;
using namespace serialization;

extern std::vector<std::vector<int> > a_15_in;

namespace BuildingOutline {

//============================================================================
class Solution {
public:
    struct SegPoint {
        int x;
        int y;
        int isStart;
        SegPoint(int x=0, int y=0, int start=0):x(x), y(y), isStart(start) {}
        SegPoint& operator=(const SegPoint& p) {
            x = p.x;
            y = p.y;
            isStart = p.isStart;
            return *this;
        }
    };
    struct PointLessByX {
    bool operator()(const SegPoint& a, const SegPoint& b) {
        return a.x < b.x;
    }
    };
    typedef multiset<SegPoint, PointLessByX> PointSetX;

    void pushOutline(vector<vector<int> > &b, int start, int end, int height) {
        vector<int> p(3);
        p[0] = start;
        p[1] = end;
        p[2] = height;
        b.push_back(p);
    }
    /**
     * @param buildings: A list of lists of integers
     * @return: Find the outline of those buildings
     */
    vector<vector<int> > buildingOutline(vector<vector<int> > &buildings) {
        // write your code here
        typedef map<int ,int> IntIntMap;
        IntIntMap pending; // <hight, count>
        PointSetX points;
        vector<vector<int> > outline;
        // construct point set in order
        for(size_t i=0; i< buildings.size(); ++i) {
            vector<int>& seg = buildings[i];
            points.insert(SegPoint(seg[0], seg[2], 1));
            points.insert(SegPoint(seg[1], seg[2], 0));
        }
        // find outline from point set
        SegPoint startP;
        for(PointSetX::iterator it=points.begin(), itUpp=points.begin(); it!=points.end();) {
            bool isEmpty = pending.empty();
            PointSetX::iterator i=it;
            for(; i->x ==it->x && i!=points.end(); ++i) {
                pending[i->y] += i->isStart? 1: (-1);
            }
            for(IntIntMap::iterator i=pending.begin(); i!=pending.end();) {
                if( i->second <= 0 ) {
                    IntIntMap::iterator i2 = i;
                    ++i;
                    pending.erase(i2);
                }else{
                    ++i;
                }
            }
            if( pending.empty() ) {
                pushOutline(outline, startP.x, it->x, startP.y);
            }else if( isEmpty ) {
                startP = SegPoint(it->x, pending.rbegin()->first, 1);
            }else if( startP.y != pending.rbegin()->first ) {
                pushOutline(outline, startP.x, it->x, startP.y);
                startP = SegPoint(it->x, pending.rbegin()->first, 1);
            }
            it = i;
        }
        return outline;
    }
};
typedef vector<int> IntVec;
typedef vector<vector<int> > IntVecVec;
void test() {
	/*
	 * Input
[[1,5,9],[2,10,3],[7,14,9],[12,18,3],[16,20,9],[20,25,19],[22,31,7]]
Output
[[1,5,9],[5,7,3],[7,14,9],[14,16,3],[16,20,9],[22,31,7]]
Expected
[[1,5,9],[5,7,3],[7,14,9],[14,16,3],[16,20,9],[20,25,19],[25,31,7]]
	 */


	IntVecVec a; //= {{1,5,9},{2,10,3},{7,14,9},{12,18,3},{16,20,9},{20,25,19},{22,31,7}};
	string text = "[[1,5,9],[2,10,3],[7,14,9],[12,18,3],[16,20,9],[20,25,19],[22,31,7]]";
	istringstream ss(text);
	ifstream ifs("15.in");
	ofstream ofs("15.out");
	ifs >> a;
	Solution sln;
	IntVecVec b = sln.buildingOutline(a);
	ofs << b;
}
} // BuildingOutline
int main()
{
	BuildingOutline::test();
    return 0;
}
