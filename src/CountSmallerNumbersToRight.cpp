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

/**
 * You are given an integer array nums and you have to return a new counts array.
 * The counts array has the property where counts[i] is the number of smaller elements to the right of nums[i].
 */
namespace CountSmallerNumbersToRight{

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

template<typename T, typename Comp=std::less<T> >
struct TreeNode {
	TreeNode *left, *right, *parent;
	T val;
	size_t valcount;
	size_t treesize; //
    Comp lessThan;

	TreeNode(TreeNode* parent, const T& v, int n=1): left(nullptr), right(nullptr), parent(parent), val(v), valcount(n), treesize(n){}

	size_t size() const {
		return treesize;
	}
	std::pair<TreeNode*, bool> insert(const T& v) {
		++treesize;
		if( lessThan(v, val) ) {
			if( left ) {
				return left->insert(v);
			}else{
				return make_pair(left = new TreeNode(this, v), true);
			}
		}else if( lessThan(val, v) ) {
			if( right ) {
				return right->insert(v);
			}else {
				return make_pair(right = new TreeNode(this, v), true);
			}
		}else{
			++valcount;
			return make_pair(this, false);
		}
	}
	// in-order previous element
	TreeNode *prev() {
		TreeNode *p;
		if( left ) {
			// find right most element
			for(p = left; p->right; p = p->right)
				;
			return p;
		}else{
			// find the first left parent
			for(p = this; p->parent; p = p->parent) {
				if( p == p->parent->right ) {
					return p->parent;
				}
			}
		}
		return nullptr;
	}
	TreeNode *next() {
		TreeNode *p;
		if( right ) {
			// find the left most one
			for(p=right; p->left; p=p->left)
				;
			return p;
		}else{
			// find the first right parent
			for(p= this; p->parent; p = p->parent) {
				if( p == p->parent->left )
					return p->parent;
			}
		}
		return nullptr;
	}
	// return in-order index of current node
	size_t index() {
		size_t n = left? left->treesize: 0;
		for(TreeNode* p = this; p->parent; p = p->parent) {
			if( p == p->parent->right ) {
				n += p->parent->treesize - p->treesize;
			}
		}
		return n;
	}
	// find the first element that not less than v
	TreeNode* lower_bound(const T& v) {
		TreeNode* p = this;
		do{
			if( lessThan(p->val, v) ) {
				if( !p->right )
					return p->next();
				p = p->right;
			}else if( lessThan(v, p->val) ) {
				if( !p->left )
					return p;
				p = p->left;
			}else{
				return p;
			}
		}while(true);
		return nullptr;
	}
	TreeNode* upper_bound(const T&v) {
		TreeNode* p = this;
		do{
			if( lessThan(p->val, v) ) {
				if( !p->right ) return p->next();
				p = p->right;
			}else if( lessThan(v, p->val) ) {
				if( !p->left ) return p;
				p = p->left;
			}else{
				return p->next();
			}
		}while(true);
	}
	TreeNode* find(const T& v) {
		if( lessThan(val, v) ) {
			return left ? left->find(v) : nullptr;
		}else if( lessThan(v, val) ) {
			return right? right->find(v) : nullptr;
		}else{
			return this;
		}
	}
	~TreeNode() {
		if( left ) {
			treesize -= left->treesize;
			delete left;
		}
		if( right ) {
			treesize -= right->treesize;
			delete right;
		}
	}
};

IntVector solve(IntVector v) {
	typedef TreeNode<int> IntTree;
	size_t N = v.size();
	IntVector r(N);
	if( N<2 ) return r;
	IntTree root(nullptr, v[N-1]);

	for(int i=N-2; i>=0; --i) {
		IntTree* p = root.lower_bound(v[i]);
		r[i] = p ? p->index() : root.size();
		root.insert(v[i]);
	}
	return r;
}
void test() {
	IntVector num{5,2,6,1};
	cout << solve(num) << endl; // [2, 1, 1, 0]
}
}

int main_CountSmallerNumbersToRight() {
	CountSmallerNumbersToRight::test();
}
