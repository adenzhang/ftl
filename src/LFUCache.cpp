#include "ContainerSerialization.h"
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
using namespace serialization;

namespace {
struct KeyValue {
    int key;
    int value;
    int freq; // frequency

    KeyValue(int k, int v): key(k), value(v), freq(0) {}
};

class LFUCache {
public:
    const int cap; // capacity

    typedef list<KeyValue> KVQue;
    typedef unordered_map<int, KVQue::iterator> KMap;

    KVQue kvq;  // front: least frequent accessed.
    KMap kmap;

    LFUCache(int capacity): cap(capacity) {
    }

    int get(int key) {
        KMap::iterator it = kmap.find(key);
        if( it == kmap.end() ) {
            return -1;
        }
        ++it->second->freq;
        adjust(it->second);
        return it->second->value;
    }

    void put(int key, int value) {
        // put will increase frequency
        if( cap <= 0 ) return;
        KMap::iterator it = kmap.find(key);
        if( it == kmap.end() ) {
            if( kvq.size() == cap ) {
                kmap.erase(kvq.front().key);
                kvq.pop_front();
            }
            kvq.push_front(KeyValue(key, value));
            it = kmap.insert(KMap::value_type(key, kvq.begin())).first;
        }else{
            it->second->value = value;
            ++ it->second->freq;
        }
        // adjust if necessary
        adjust(it->second);
    }
    void adjust(KVQue::iterator& it) {
        KVQue::iterator next = it;
        // find next.freq > it->freq
        while( next != kvq.end() && it->freq >= next->freq ) {
            ++next;
        }
        if( it != next )
            kvq.splice(next, kvq, it); //swap position
    }
};

/**
 * Your LFUCache object will be instantiated and called as such:
 * LFUCache obj = new LFUCache(capacity);
 * int param_1 = obj.get(key);
 * obj.put(key,value);
 */

std::ostream& operator<<( std::ostream& os, const LFUCache &cache) {
    printList(os, cache.kvq);
    return os;
}
std::ostream& operator<<( std::ostream& os, const KeyValue &e) {
    os << "(" << e.key << "," << e.value <<", " << e.freq <<  ")";
    return os;
}

}

int main_LFUCache()
{

//    LFUCache cache( 2 /* capacity */ );
//    cache.put(1, 1);
//    cache.put(2, 2);
//    cout << cache.get(1) << endl;       // returns 1
//    cout << cache << endl;
//    cache.put(3, 3);    // evicts key 2
//    cout << cache.get(2) << endl;       // returns -1 (not found)
//    cout << cache.get(3) << endl;       // returns 3.
//    cache.put(4, 4);    // evicts key 1.
//    cout << cache.get(1) << endl;       // returns -1 (not found)
//    cout << cache.get(3) << endl;       // returns 3
//    cout << cache.get(4) << endl;       // returns 4

//    LFUCache cache(3);
//    cache.put(2, 2);
//    cache.put(1, 1);
//    cout << cache.get(2) << endl;
//    cout << cache.get(1) << endl;
//    cout << cache.get(2) << endl;
//    cache.put(3, 3);
//    cout << cache << endl;
//    cache.put(4, 4);
//    cout << cache.get(3) << endl;  // -1
//    cout << cache.get(2) << endl;

    LFUCache cache(2);
    cache.put(3, 1);
    cache.put(2, 1);
    cache.put(2, 2);
    cache.put(4, 4);
    cout << cache.get(2) << endl;

//    LFUCache cache(0);
//    cache.put(0,0);
//    cout << cache.get(0) << endl;

    cout << "hello" << endl;
}
