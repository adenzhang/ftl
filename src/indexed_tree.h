#ifndef _INDEXED_TREE_H_
#define _INDEXED_TREE_

#include <vector>
#include <functional>

// https://www.topcoder.com/community/data-science/data-science-tutorials/binary-indexed-trees/
/*
Binary Indexed Tree or Fenwick Tree
Let us consider the following problem to understand Binary Indexed Tree.

We have an array arr[0 . . . n-1]. We should be able to
1 Find the sum of first i elements.
2 Change value of a specified element of the array arr[i] = x where 0 <= i <= n-1.

A simple solution is to run a loop from 0 to i-1 and calculate sum of elements.
To update a value, simply do arr[i] = x. The first operation takes O(n) time and second operation takes O(1) time.
Another simple solution is to create another array and store sum from start to i at the i’th index in this array.
Sum of a given range can now be calculated in O(1) time, but update operation takes O(n) time now.
This works well if the number of query operations are large and very few updates.
 */


namespace ftl {

template <typename ValueType, typename AccType=std::plus<ValueType>, typename ContainerType = std::vector<ValueType> >
class indexed_tree {
public:
    // public types
	typedef ValueType value_type;
	typedef AccType AccFunc;
//    typedef std::binary_function<const ValueType&, const ValueType&, ValueType> AccFunc;
protected:
	AccFunc accFunc;
    ContainerType biTree;
public:
    indexed_tree(size_t size=0, const AccFunc& func=AccType())
        : accFunc(func)
        , biTree(size+1)
    {}

    indexed_tree(typename ContainerType::iterator it, typename ContainerType::iterator itEnd
    		, const AccFunc& func=AccType())
        : accFunc(func)
    {
    	construct(it, itEnd);
    }

    indexed_tree(typename ContainerType::iterator it, size_t N
    		, const AccFunc& func=AccType())
        : accFunc(func)
    {
    	construct(it, N);
    }

    void resize(size_t size) {
        biTree.resize(size+1);
    }
    void add(size_t pos, const ValueType& v) {
    	// index in BITree[] is 1 more than the index in arr[]
    	++pos;
    	const size_t N = biTree.size();
    	while(pos <= N) {
    		biTree[pos] = accFunc(v, biTree[pos]);
    		pos += pos & (-pos); // add the last digit 1 to get higher level position.
    	}
    }

    template<typename Iterator>
    void construct(Iterator it, const size_t N=0) {
    	resize(N);
    	for(size_t i=0; i<N; ++i, ++it) {
    		add(i, *it);
    	}
    }

    template<typename Iterator>
    void construct(Iterator it, Iterator itEnd) {
    	resize(std::distance(it, itEnd));
    	for(size_t i=0; it != itEnd; ++it, ++i) {
    		add(i, *it);
    	}
    }

    // Returns sum of arr[0..index]. This function assumes
    // that the array is preprocessed and partial sums of
    // array elements are stored in BITree[].
    ValueType getResult(size_t n) const {
    	ValueType sum;
    	++n;
    	while( n > 0 ) {
    		sum = accFunc(biTree[n], sum);
    		n -= n & (-n); // remove last bit 1 to get parent position.
    	}
    	return sum;
    }

    ContainerType& getBiTree() {
    	return biTree;
    }
};

template <typename KeyType, typename ValueType>
class BinaryTreeType {
public:
	// types
	typedef std::pair<KeyType, ValueType> value_type;
	enum ICHILD {ILEFT=0, IRIGHT=1, NUM_CHILDREN = 2};
	struct Node {
		value_type kv;
		Node *children[NUM_CHILDREN], *parent;
		Node(const value_type& kv)
		: kv(kv), parent(NULL)
		{
			for(int i=0; i<NUM_CHILDREN; ++i)
				children[i] = NULL;
		}
	};
	static Node* addChild(Node *parent, Node *child, int iRight) {
		child->children[iRight] = parent->children[iRight];
		if( child->children[iRight] ) child->children[iRight]->parent = child;
		child->parent = parent;
		parent->children[iRight] = child;
		return child;
	}

	// isNext, 1 or 0 (previous)
	// return next node in PreOrder
	static Node *nextNode(Node* p, int isNext=1) {
		if( !p ) return p;
		int iRight = isNext;
		int iLeft = isNext? 1: 0;
		if( p->children[iRight] ) { // go to left most leaf of right subtree
			p = p->children[iRight];
			while( p->children[iLeft] ) p = p->children[iLeft];
			return p;
		} else { // go up until it's a left child or it's the root.
			for( ;p->parent; p = p->parent ) {
				if( p == p->parent->children[iLeft] ) { // it's the left child of parent
					return p->parent;
				}  // else it's the right child of parent
			}
			// now p is root
			return NULL;
		}
	}
	struct iterator {
		Node *node;
		iterator(Node *node = NULL): node(node)
		{}
		value_type* operator->() {
			return node->kv;
		}
		value_type& operator*() {
			return node->kv;
		}
		iterator& operator++() {
			node = nextNode(node);
			return *this;
		}
		iterator operator++(int) {
			iterator it(node);
			node = nextNode(node);
			return it;
		}
		iterator& operator--() {
			node = nextNode(node, 0);
			return *this;
		}
		iterator operator--(int) {
			iterator it(node);
			node = nextNode(node, 0);
			return it;
		}
		iterator parent() const {
			return iterator(node ? node->parent : node);
		}
		iterator child(int idx) const {
			return iterator(node ? node->children[idx] : node);
		}
		iterator left() const {
			return iterator(node ? node->children[ILEFT]: node);
		}
		iterator right() const {
			return iterator(node ? node->children[IRIGHT] : node);
		}
		bool hasNext() const {
			return nextNode(node);
		}
		bool hasPrevious() const {
			return nextNode(node, 0);
		}
	};
	template<typename VisitFunc>
	static void visit_by_preorder(iterator root, const VisitFunc& func) {
		// todo
	}
};

template <typename KeyType, typename ValueType, typename KeyCompare=std::less<KeyType> >
class binary_search_tree : public BinaryTreeType<KeyType, ValueType> {
	// private types
public:
	// public types
	typedef BinaryTreeType<KeyType, ValueType> SuperType;
	typedef typename SuperType::Node Node;
	typedef typename SuperType::iterator iterator;
	typedef typename SuperType::value_type value_type;

//	using BinaryTreeType<KeyType, ValueType>::Node;
	enum {ILEFT=0, IRIGHT=1, NUM_CHILDREN = 2};
protected:
	// private members
	Node           *_root;
	size_t          _size;
	KeyCompare      _keyLess;
public:
	// public functions
	binary_search_tree(const KeyCompare& kc = std::less<KeyType>()): _root(nullptr), _size(0), _keyLess(kc) {}

	iterator root() {
		return iterator(_root);
	}
	iterator begin() {
		Node *p = _root;
		if( p )
		while( p->children[ILEFT] ) p = p->children[ILEFT];
		return iterator(p);
	}
	iterator last() {
		Node *p = _root;
		if( p )
		while( p->children[IRIGHT] ) p = p->children[IRIGHT];
		return iterator(p);
	}
	iterator end() {
		return iterator();
	}
	iterator lower_bound(const KeyType& v) {
		// return the first element that >= v
		Node *p = _root;
		if( p && !_keyLess(p->kv.first, v) ) {
			while( true ) {
				Node *prev = nextNode(p, 0);
				if( prev && !_keyLess(prev->kv.first, v) ) {
					p = prev;
				}else{
					return iterator(p);
				}
			}
		}
		for(; p && _keyLess(p->kv.first, v); p = nextNode(p) ) ;
		return iterator(p);
	}
	iterator upper_bound(const KeyType& v) {
		// return the first element that > v
		Node *p = _root;
		if( p && _keyLess(v, p->kv.first) ) {
			while( true ) {
				Node *prev = nextNode(p, 0);
				if( prev && _keyLess(v, prev->kv.first) ) {
					p = prev;
				}else{
					return iterator(p);
				}
			}
		}
		for(; p && !_keyLess(v, p->kv.first); p = nextNode(p) );
		return iterator(p);
	}
	std::pair<iterator, bool> insert(const value_type& kv) {
		if( !_root ) {
			_root = new Node(kv);
			return std::make_pair(_root, true);
		}
		Node *p = _root;
		while(true) {
			if( _keyLess(p->kv.first, kv.first) ) {
				if( p->children[IRIGHT] ) {
					p = p->children[IRIGHT];
				}else{
					p = SuperType::addChild(p, new Node(kv), IRIGHT);
					break;
				}
			}else if( _keyLess(kv.first, p->kv.first) ) {
				if( p->children[ILEFT] ) {
					p = p->children[ILEFT];
				}else{
					p = SuperType::addChild(p, new Node(kv), ILEFT);
					break;
				}
			}else{
				return std::make_pair(iterator(p), false);
			}
		}
		return std::make_pair(p, true);
	}
};
}


#endif //
