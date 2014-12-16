#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>

using namespace std;

template<typename DataType, typename ExtraType>
struct Node {
    typedef std::shared_ptr<Node> NodePtr;
    NodePtr left, right, parent;
    DataType d;
    ExtraType ex;
    Node(const DataType& a): d(a) {}
};

struct NodeDistanceSum {
    size_t leftSum, rightSum;  // the distance sum from the current root to all left/right subtree nodes
    size_t leftCount, rightCount;
};

typedef int DataType;
typedef Node<DataType, NodeDistanceSum> BstNode;
typedef typename BstNode::NodePtr BstNodePtr;
typedef std::vector<DataType> DataVec;

BstNodePtr addNode(BstNodePtr parent, BstNodePtr& node, DataType x) {
    node.reset(new BstNode(x));
    node->parent = parent;
    return node;
}
BstNodePtr push_BST(BstNodePtr& root, DataType x)
{
    if( !root ) {
        return addNode(BstNodePtr(), root, x);
    }
    BstNodePtr p =  root;
    while(true){
        if(p->d < x ) {
            if( p->right ) p = p->right;
            else return addNode(p, p->right, x);
        }else if( p->d > x ) {
            if( p->left) p = p->left;
            else return addNode(p, p->left, x);
        }else {
            return BstNodePtr();
        }
    }
    return BstNodePtr();
}
int udpateDistance(int dist, BstNodePtr root, BstNodePtr n, int depth = 0)
{
    BstNodePtr pa = n->parent;
    if( !pa ) return 0;
    size_t otherSum = 0, otherCount = 0;
    if( pa->left == n ) {
        pa->ex.leftSum += depth+1;
        pa->ex.leftCount++;
        otherSum = pa->ex.rightSum;
        otherCount = pa->ex.rightCount;
    }else{
        pa->ex.rightSum += depth+1;
        pa->ex.rightCount++;
        otherSum = pa->ex.leftSum;
        otherCount = pa->ex.leftCount;
    }
    dist = dist+otherSum + (otherCount+1) * (depth+1);
    if( pa != root ) {
        return udpateDistance(dist , root, pa, depth+1);
    }else{
        return dist;
    }
}
void distance_maintain()
{

    DataVec v = {4, 7, 3, 1, 8, 2, 6, 5};
    BstNodePtr root;
    size_t sum = 0;
    for(DataVec::iterator it=v.begin(); it!= v.end(); ++it) {
        BstNodePtr newNode = push_BST(root, *it);
        if( newNode ) {
            sum += udpateDistance(0, root, newNode, 0);
            cout << sum << endl;
        }
    }
}
