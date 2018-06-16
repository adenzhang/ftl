#include <stdio.h>
#include <vector>
#include <iostream>

using namespace std;

struct Point {
     int x;
     int y;
     Point() : x(0), y(0) {}
     Point(int a, int b) : x(a), y(b) {}
};

/**
 * Definition for a point.
 * struct Point {
 *     int x;
 *     int y;
 *     Point() : x(0), y(0) {}
 *     Point(int a, int b) : x(a), y(b) {}
 * };
 */

class Solution {
public:
    bool isOnLine(Point& p1, Point& p2, Point& p3) {
        //return (p3.y-p1.y)/(p3.x-p1.x) == (p2.y-p1.y)/(p2.x - p1.x)
        return (p3.y-p1.y) * (p2.x - p1.x) == (p2.y-p1.y) * (p3.x-p1.x);
    }
    bool isSamePoint(Point& p1, Point& p2) {
        return p1.x == p2.x && p1.y == p2.y;
    }
    int maxPoints(vector<Point> &points) {
        const int N = points.size();
        vector<int> ndup(N);
        for(int i=0; i<N; ++i)
            ndup[i] = 1;
        int nn = N;
        for(int i=0; i<nn-1; ++i) {
            for(int j=i+1; j<nn; ++j) {
                if( isSamePoint(points[i], points[j]) ) {
                    points.erase(points.begin()+j);
                    ndup[i]++;
                    nn--;
                    j--;
                }
            }
        }
        int m = N==0?0:(N==1?1:ndup[0]);
        for(int i=0; i<nn-1; ++i) {
            if( ndup[i] > m)
                m = ndup[i];
            for(int j=i+1; j<nn; ++j) {
                int mt = ndup[i] + ndup[j];
                for(int k=j+1; k<nn; ++k) {
                    if( isOnLine(points[i], points[j], points[k]) )
                        mt += ndup[k];
                }
                if( mt > m)
                    m = mt;
            }
        }
        return m;
    }
};

int main_maxPoints()
{
    //Point a[]= {Point(0,0), Point(1,1), Point(0,0)};
    vector<Point> v;//v(&a[0], &a[2]);
    v.push_back(Point(0,0));
    v.push_back(Point(1,1));
    v.push_back(Point(0,0));
    Solution s;
    cout << s.maxPoints(v) << endl;
    return 0;
}
