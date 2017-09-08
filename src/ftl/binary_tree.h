#ifndef BINARY_TREE_H
#define BINARY_TREE_H

namespace ftl {

template <typename KeyType, typename ValueType>
class binary_tree {
public:
	// types
	typedef std::pair<KeyType, ValueType> value_type;
	enum ICHILD {ILEFT=0, IRIGHT=1, NUM_CHILDREN = 2};
	struct Node {
		value_type kv;
		Node *children[NUM_CHILDREN], *parent;
		Node(const value_type& kv)
		: kv(kv), parent(nullptr)
		{
			for(int i=0; i<NUM_CHILDREN; ++i)
				children[i] = nullptr;
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
	// return next node in InOrder
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
			return nullptr;
		}
	}

	// Inorder iterator
	struct iterator {
		Node *node;
		iterator(Node *node = nullptr): node(node)
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
};

}
#endif
