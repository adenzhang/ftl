#include <list>
#include <algorithm>
#include <iostream>
#include <map>

using namespace std;


struct KeyValue {
    int key;
    int value;
    KeyValue(int k, int v): key(k), value(v) {}
};

class LRUCache{
public:

    typedef list<KeyValue> FreqSet;
    typedef map<int, FreqSet::iterator > FMap;
    FMap fmap;
    FreqSet fset; // back is new, front is old
    const int   cap;

    LRUCache(int capacity): cap(capacity) {

    }
    int get(int key) {
        FMap::iterator itm = fmap.find(key);
        if( itm == fmap.end() )
            return -1;

        int v = -1;
        FreqSet::iterator it = itm->second;
        v = it->value;
        fset.push_back(*it);
        fset.erase(it);
        it = fset.end();
        --it;
        itm->second = it;

        return v;
    }
    void set(int key, int value) {
        FMap::iterator itm = fmap.find(key);
        FreqSet::iterator it;
        if( itm == fmap.end() ) {
            fset.push_back(KeyValue(key,value));
            it = fset.end();
            --it;
            fmap[key] = it;
        }else{
            it = itm->second;
            it->value = value;
            fset.push_back(*it);
            fset.erase(it);
            it = fset.end();
            --it;
            itm->second = it;

            return;
        }
        if( fmap.size() > cap ) {
            // remove LRU
            int rmKey = fset.front().key;
            fset.pop_front();
            fmap.erase(rmKey);
        }
    }
};

int main_LRUCache()
{
    LRUCache lru(2);

//    cout << lru.get(1) << endl;  // -1
    lru.set(2,1);
    lru.set(1,1);
    lru.set(2,3);
    lru.set(4,1);
    cout << lru.get(1) << endl;
    cout << lru.get(2) << endl;

    return 0;
}
