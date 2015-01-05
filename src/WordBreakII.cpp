
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


// all letters should be converted to upper case before adding to trie
class TrieEntry {
public:
    enum {MAX_CHILDREN = 50};

    typedef TrieEntry* TrieEntryPtr;
public:
    TrieEntry(char c=0, int aleaf=0, TrieEntryPtr par=TrieEntryPtr()): ch(c), leafType(aleaf), parent(par) {
        for(int i=0; i<MAX_CHILDREN; ++i) children[i] = TrieEntryPtr();
    }
    ~TrieEntry() {
        clearChildren();
    }

    // return leaf node of the added word
    TrieEntryPtr addWord(const char* word, int leaftype=1 ) {
        if( !word || !*word) return TrieEntryPtr();

        int id = IDX(*word);
        if( id <0 || id>=MAX_CHILDREN )
            return TrieEntryPtr();
        TrieEntryPtr p = children[id]? children[id]: createChild(*word, word[1]?0:leaftype);
        return word[1]? p->addWord(word+1, leaftype) : ((p->leafType = leaftype), p);
    }
    TrieEntryPtr follow(char c) const {
        return children[IDX(c)];
    }
    // get word exclusive of current entry
    string retrieveWord(TrieEntryPtr descendant) const {
        string s;
        if( !descendant ) return s;
        for(TrieEntryPtr p = descendant; p != this; p = p->parent)
            s += p->ch;
        // reverse
        int L = s.length();
        for(int i=0; i< L>>1; ++i)
            std::swap(s[i], s[L-i-1]);
        return s;
    }

    // match exactly the word
    TrieEntryPtr matchWord(const char* word) const {
        if( !word ) return TrieEntryPtr();
        TrieEntryPtr p=(TrieEntryPtr)this;
        for(; p && *word; (p=p->follow(*word)), ++word) {}
        return p ;
    }

    // try to match prefixes of word as many as possible
    // @len the longest prefix of word to be searched.
    vector<TrieEntryPtr> matchPrefixes(const char* word, int len=0) const {
        vector<TrieEntryPtr> result;
        if( !word ) return result;
        TrieEntryPtr p=(TrieEntryPtr)this;
        if( len == 0) len = strlen(word);
        int k = 0;
        for(; p && *word && k<len; (p=p->follow(*word)), ++word,  ++k) {
            if( p->leafType )
                result.push_back(p);
        }
        if( p && p->leafType )
            result.push_back(p);
        return result;
    }
    vector<std::string> matchAll(const char* word, int len=0) const {
        vector<TrieEntryPtr> all = matchPrefixes(word, len);
        vector<std::string> alls(all.size());
        for(int i=0; i<all.size(); ++i) {
            alls[i] = retrieveWord(all[i]);
        }
        return alls;
    }

    void clearChildren() {
        for(int i=0; i<MAX_CHILDREN; ++i) {
            if(children[i]) {
                children[i]->clearChildren();
                delete children[i];
                children[i] = TrieEntryPtr();
            }
        }
    }
    int leaf() {
        return leafType;
    }

protected:
    int          leafType;   // by default, 0 for inner node, 1 for leaf
    char          ch;        // if ch == 0, it's a root
    TrieEntryPtr parent;
    TrieEntryPtr children[MAX_CHILDREN];

    TrieEntryPtr createChild(char c=0, int aleaf=0) {
        int id = IDX(c);
        if( id <0 || id>=MAX_CHILDREN )
            return TrieEntryPtr();
        return children[id] = TrieEntryPtr(new TrieEntry(c, aleaf, this));  // should be weak_ptr
    }
    static inline int IDX(char c) { return  c- '0';}

};
typedef TrieEntry* TrieEntryPtr;

class Solution {

public:

};

static void test() {
    TrieEntry root;
    root.addWord("DOG");
    root.addWord("LOG");
    root.addWord("DOT");
    root.addWord("DOGIE");

    cout << root.retrieveWord(root.matchWord("LOG")) << endl;
    typedef vector<string> StringVec;
    StringVec sv;
    sv = root.matchAll("DOGIESSS");
    for(int i=0; i<sv.size(); ++i) {
        cout << sv[i] << endl;
    }

}

int main(int, const char **) {
    test();
    return 0;
}
