#include <vector>
#include <algorithm>

using namespace std;
/*
struct Interval {
    int start;
    int end;
    Interval() : start(0), end(0) {}
    Interval(int s, int e) : start(s), end(e) {}
};

struct CompareInterval {
    bool operator()(Interval& i1, Interval& i2) {
        return i1.start < i2.start;
    }
};

/**
 * Definition for an interval.
 * struct Interval {
 *     int start;
 *     int end;
 *     Interval() : start(0), end(0) {}
 *     Interval(int s, int e) : start(s), end(e) {}
 * };
 */
/*
class Solution {
public:
    typedef vector<Interval> IntervalList;
    vector<Interval> insert(vector<Interval> &intervals, Interval newInterval) {
        IntervalList::iterator it = std::lower_bound(intervals.begin(), intervals.end(), newInterval, CompareInterval());
        if( it == intervals.end() ) {
            intervals.push_back(newInterval);;
            return IntervalList(intervals.begin(), intervals.end());
        }
        IntervalList::iterator it0 = it;
        if( it0 != intervals.begin() && it0->start > newInterval.start ) {
            it0--;
            if( it0->end < newInterval.start )
                it0++;
        }

        IntervalList::iterator it1 = std::upper_bound(it0, intervals.end(), Interval(newInterval.end, newInterval.end+1), CompareInterval());

        int newEnd = std::max(it->end, newInterval.end);
    }
};
*/
