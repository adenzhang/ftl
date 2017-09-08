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
#include <ftl/container_serialization.h>
using namespace std;
using namespace ftl::serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

namespace MergeSortedLists {

struct ListNode {
    int val;
    ListNode *next;
    ListNode(int x) : val(x), next(NULL) {}
};

class Solution {
public:
    struct NodeGreater{
        bool operator()(ListNode* x, ListNode* y) const {
            return x->val > y->val;
        }
    };

    ListNode* mergeKLists(vector<ListNode*>& lists) {
        typedef priority_queue<ListNode*, vector<ListNode*>, NodeGreater> NodeQ;
        NodeQ curLists;
        const int N = lists.size();
        for(int i=0; i<N; ++i) {
            if(lists[i]) curLists.push(lists[i]);
        }
        if( curLists.empty() ) return NULL;

        ListNode* head = curLists.top();
        ListNode* tail = head;
        do {
            curLists.pop();
            if(tail->next) curLists.push(tail->next);
            if( curLists.empty() ) {
                break;
            } else {
                tail->next = curLists.top();
                tail = tail->next;
            }
        } while(true);
        tail->next = NULL;
        return head;
    }
};

}

int main_MergeSortedLists() {
}

