
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
    enum {MAX_CHILDREN = 30, CH_START='a'};

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
        for(TrieEntryPtr p = descendant; p != this_ptr(); p = p->parent)
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
        TrieEntryPtr p=this_ptr();
        for(; p && *word; (p=p->follow(*word)), ++word) {}
        return p ;
    }

    // try to match prefixes of word as many as possible
    // @len the longest prefix of word to be searched.
    vector<TrieEntryPtr> matchPrefixes(const char* word, int len=0) const {
        vector<TrieEntryPtr> result;
        if( !word ) return result;
        TrieEntryPtr p=this_ptr();
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
        return std::move(alls);
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
        return children[id] = TrieEntryPtr(new TrieEntry(c, aleaf, this_ptr()));  // should be weak_ptr
    }
    static inline int IDX(char c) { return  c-CH_START;}
    TrieEntryPtr this_ptr() const { return TrieEntryPtr(this);}

};
typedef TrieEntry* TrieEntryPtr;


static void test_trie() {
    TrieEntry root;
    root.addWord("DOG");
    root.addWord("LOG");
    root.addWord("DOT");
    root.addWord("DOGIE");

    cout << root.retrieveWord(root.matchWord("LOG")) << endl;
    typedef vector<string> StringVec;
    StringVec sv;
    sv = root.matchAll("DOGIESSS", 5);
    for(int i=0; i<sv.size(); ++i) {
        cout << sv[i] << endl;
    }

}
//===================================================================
template<typename DataType>
struct LinkedNode {
    typedef LinkedNode* LinkedNodePtr;
//    LinkedNodePtr parent;
    typedef std::vector<LinkedNodePtr> NodeList;
    NodeList children;
    DataType data;
    LinkedNode(){}

    LinkedNode(const DataType& d):data(d) {}

    LinkedNodePtr createChild(const DataType& d) {
        LinkedNodePtr ch(new LinkedNode(d));
        addChild(ch);
        return ch;
    }

    void addChild(LinkedNodePtr node) {

        if( node ) {
            children.push_back(node);
        }else{
            children.clear();
        }
    }
    bool removeChild(LinkedNodePtr node) {
        // todo
        return false;
    }

};

typedef LinkedNode<std::pair<const char*, int> > DynNode;
typedef typename DynNode::LinkedNodePtr DynNodePtr;
typedef vector<DynNodePtr> DynNodeVector;

class Solution {
public:
    enum {NOT_VISITED=0, OK_LEAF=0x01, OK_LINK=0x02, OK_BRANCH=0x04};
    typedef unordered_set<string> StringSet;
    typedef vector<string>  StringVector;
    typedef unordered_map<const char *, DynNodePtr>  PtrNodeMap;
    vector<string> wordBreak(const string s, const unordered_set<string> &dict) {
        vector<string> result;
        TrieEntry root;
        for(StringSet::const_iterator it=dict.begin(); it!=dict.end(); ++it)
            root.addWord(it->c_str());

        DynNode dynTree;
        PtrNodeMap completedPos;
        nextWord(s.c_str(), root, DynNodePtr(&dynTree), completedPos);
        result = outputWords(dynTree.children.front());

//        nextWord(s.c_str(), root, "", result);
        return result;
    }
    bool nextWord(const char *s, TrieEntry& trie, DynNodePtr words, PtrNodeMap& completedPos) {
        if( !s || ! *s)
            return false;
        StringVector sv = trie.matchAll(s);
        DynNodePtr thisNode = words->createChild(make_pair(s, NOT_VISITED));
        completedPos[s] = thisNode;
        for(int i=0; i<sv.size(); ++i) {
            string &t = sv[i];
            if( s[t.length()] ) {
                PtrNodeMap::iterator itChild = completedPos.find(s+t.length());
                if( itChild == completedPos.end() ) {
                    if( nextWord(s + t.length(), trie, thisNode, completedPos) ) {
                        thisNode->data.second |= OK_BRANCH;
                    }
                }else{
                    thisNode->addChild(itChild->second);
                    if( itChild->second->data.second )
                        thisNode->data.second |= OK_LINK;
                }
            }else{
                thisNode->data.second |= OK_LEAF;
//                cout << "found it" << endl;
            }
        }
        return thisNode->data.second;
    }
    StringVector outputWords(DynNodePtr node) {
        StringVector result;
        for(DynNode::NodeList::iterator it = node->children.begin(); it != node->children.end(); ++it) {
            if( (*it)->data.second ) {
                StringVector r = outputWords((*it));
                if( r.size() > 0 ) {
                    string s(node->data.first, (*it)->data.first);
                    for(int i=0; i<r.size(); ++i)
                        result.push_back(s + " " + r[i]);
                }
            }
        }
        if( node->data.second & OK_LEAF) {
            result.push_back(string(node->data.first));
        }
        return std::move(result);
    }

    void nextWord0(const char *s, TrieEntry& trie, const string words, StringVector& result) {
        if( !s || ! *s)
            return;
        StringVector sv = trie.matchAll(s);
        for(int i=0; i<sv.size(); ++i) {
            string &t = sv[i];
            if( s[t.length()] ) {
                nextWord0(s + t.length(), trie, words + " " + t, result);
            }else{
                result.push_back(words + " " + t);
            }
        }
    }

    static void test(const string s, const unordered_set<string> &dict) {
        Solution sln;
        vector<string> vs = sln.wordBreak(s, dict);
        cout << vs.size() << "[";
        for(int i=0; i<vs.size(); ++i) {
            cout <<"\"" << vs[i] << "\",";
        }
        cout << "]" << endl;
    }
};

static void test() {
    Solution::test("catsanddog", {"cat", "cats", "and", "sand", "dog"}); // "cats and dog", "cat sand dog"

    Solution::test(
        "aaaa", {"aaaa","aa","a"}
        );

    Solution::test(
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab"
            , {"a","aa","aaa","aaaa","aaaaa","aaaaaa","aaaaaaa","aaaaaaaa","aaaaaaaaa","aaaaaaaaaa"}
    );

    Solution::test(
        "aaaaaaa", {"aaaa","aa","a"}
        );
    cout << vector<string>({"a a a a a a a","aa a a a a a","a aa a a a a","a a aa a a a","aa aa a a a","aaaa a a a","a a a aa a a","aa a aa a a","a aa aa a a","a aaaa a a","a a a a aa a","aa a a aa a","a aa a aa a","a a aa aa a","aa aa aa a","aaaa aa a","a a aaaa a","aa aaaa a","a a a a a aa","aa a a a aa","a aa a a aa","a a aa a aa","aa aa a aa","aaaa a aa","a a a aa aa","aa a aa aa","a aa aa aa","a aaaa aa","a a a aaaa","aa a aaaa","a aa aaaa"}
    ).size() << endl;

}

int main_WordBreakII(int, const char **) {
    test();
    return 0;
}
