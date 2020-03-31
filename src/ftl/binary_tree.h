/*
 * This file is part of the ftl (Fast Template Library) distribution (https://github.com/adenzhang/ftl).
 * Copyright (c) 2018 Aden Zhang.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <deque>
#include <ftl/stdconcept.h>
#include <ftl/object_member.h>
namespace ftl
{

//============ TreeNode and util functions ======================
template<class T>
struct TreeNode
{
    T val;
    TreeNode *left = nullptr;
    TreeNode *right = nullptr;

    //    DEF_ATTRI( data, T, val, T() )
    //    DEF_ATTRI( left, TreeNode *, left, nullptr )
    //    DEF_ATTRI( right, TreeNode *, right, nullptr )

    DEF_ATTRI_ACCESSOR( data, val )
    DEF_ATTRI_ACCESSOR( left, left )
    DEF_ATTRI_ACCESSOR( right, right )

    DEF_ATTRI( parent, TreeNode *, parent, nullptr )

    int flag = 0;
    DEF_ATTRI_GETTER( flag, flag )
    DEF_ATTRI_SETTER( flag, flag )

    ~TreeNode()
    {
        if ( left )
        {
            delete left;
            left = nullptr;
        }
        if ( right )
        {
            delete right;
            right = nullptr;
        }
    }
};

template<class It,
         class ValConvert,
         class Node = TreeNode<decltype( std::declval<ValConvert>()( *std::declval<It>() ) )>,
         class F = std::remove_const_t<std::remove_reference_t<decltype( *std::declval<It>() )>>>
Node *create_tree( It itBegin, It itEnd, const ValConvert &getVal, const F &nullValOrTester )
{
    using RawValT = std::remove_const_t<std::remove_reference_t<decltype( *std::declval<It>() )>>;
    //    using ValT = decltype( std::declval<ValConvert>()( *std::declval<It>() ) );

    if ( itBegin == itEnd )
        return nullptr;

    const auto &IsNullVal = [&]( const RawValT &v ) -> bool {
        if constexpr ( std::is_constructible_v<F, RawValT> )
            return v == nullValOrTester;
        else
            return nullValOrTester( v );
    };

    assert( !IsNullVal( *itBegin ) );
    auto pRoot = new Node{getVal( *itBegin )};

    std::deque<Node *> q;
    q.push_back( pRoot );
    auto it = std::next( itBegin );
    while ( !q.empty() && it != itEnd )
    {
        auto p = q.front();
        q.pop_front();

        // left child
        if ( !IsNullVal( *it ) )
        {
            p->left = new Node{getVal( *it )};
            q.push_back( p->left );
        }
        // rifht child
        if ( ++it == itEnd )
            break;
        if ( !IsNullVal( *it ) )
        {
            p->right = new Node{getVal( *it )};
            q.push_back( p->right );
        }
        ++it;
    }
    return pRoot;
}

template<class It,
         class Node = TreeNode<std::remove_const_t<std::remove_reference_t<decltype( *std::declval<It>() )>>>,
         class F = std::remove_const_t<std::remove_reference_t<decltype( *std::declval<It>() )>>>
Node *create_tree( It itBegin, It itEnd, const F &nullValOrTester )
{
    using ValT = std::remove_const_t<std::remove_reference_t<decltype( *std::declval<It>() )>>;
    return create_tree(
            itBegin, itEnd, []( const ValT &v ) { return v; }, nullValOrTester );
}

template<class Inserter, class Node, class ValConvert, class NullVal>
size_t insert_from_tree( Inserter it, const Node *pRoot, const ValConvert &getVal, const NullVal &nullval )
{
    using OutValT = typename Inserter::container_type::value_type;

    size_t count = 0;
    if ( !pRoot )
        return count;

    const auto getData = [&]( const Node &n ) -> OutValT { return getVal( n.val ); }; // todo : node::get_data

    ++count;
    it = getData( *pRoot );
    std::deque<const Node *> q;
    q.push_back( pRoot );

    while ( !q.empty() )
    {
        for ( size_t i = 0, N = q.size(); i < N; ++i )
        {
            auto p = q.front();
            q.pop_front();

            if ( p->left )
            {
                it = getData( *p->left );
                q.push_back( p->left );
                ++count;
            }
            else
                it = nullval;
            if ( p->right )
            {
                it = getData( *p->right );
                q.push_back( p->right );
                ++count;
            }
            else
                it = nullval;
        }
        // don't visit the last level
        bool hasChild = false;
        for ( auto p : q )
        {
            if ( p->left || p->right )
            {
                hasChild = true;
                break;
            }
        }
        if ( !hasChild )
            break;
    }
    return count;
}

template<class Inserter, class Node, class NullVal>
size_t insert_from_tree( Inserter it, const Node *pRoot, const NullVal &nullval )
{
    using OutValT = typename Inserter::container_type::value_type;

    return insert_from_tree(
            it, pRoot, []( const OutValT &v ) { return v; }, nullval );
}

/*********************************
TEST_CASE( "TreeNode_tests" )
{
    {
        const int NULLV = INT_MAX;
        std::vector<int> v = {1, 2, 3, 4, NULLV, NULLV, NULLV, 5, 6, NULLV, NULLV, NULLV, 7};
        auto pRoot = create_tree( v.begin(), v.end(), NULLV );
        std::vector<int> u;
        auto res = insert_from_tree( std::back_inserter( u ), pRoot, NULLV );
        REQUIRE( res == 7 );
        delete pRoot;
    }
    {
        const std::string NULLV = "null";
        std::vector<std::string> v = {"1", "2", "3", "4", NULLV, NULLV, NULLV, "5", "6", NULLV, NULLV, NULLV, "7"};
        auto pRoot = create_tree( v.begin(), v.end(), []( const std::string &v ) -> int { return atoi( v.c_str() ); }, NULLV );
        REQUIRE( pRoot != nullptr );

        std::vector<int> u;
        auto res = insert_from_tree( std::back_inserter( u ), pRoot, INT_MAX );
        REQUIRE( res == 7 );

        std::vector<std::string> w;
        res = insert_from_tree( std::back_inserter( w ), pRoot, []( int v ) { return std::to_string( v ); }, NULLV );
        REQUIRE( res == 7 );
    }
};
  ******************/
//============= binary_tree ===========================

template<typename KeyType, typename ValueType>
class binary_tree
{
public:
    // types
    typedef std::pair<KeyType, ValueType> value_type;
    enum ICHILD
    {
        ILEFT = 0,
        IRIGHT = 1,
        NUM_CHILDREN = 2
    };
    struct Node
    {
        value_type kv;
        Node *children[NUM_CHILDREN], *parent;
        Node( const value_type &kv ) : kv( kv ), parent( nullptr )
        {
            for ( int i = 0; i < NUM_CHILDREN; ++i )
                children[i] = nullptr;
        }
    };
    static Node *addChild( Node *parent, Node *child, int iRight )
    {
        child->children[iRight] = parent->children[iRight];
        if ( child->children[iRight] )
            child->children[iRight]->parent = child;
        child->parent = parent;
        parent->children[iRight] = child;
        return child;
    }

    // isNext, 1 or 0 (previous)
    // return next node in InOrder
    static Node *nextNode( Node *p, int isNext = 1 )
    {
        if ( !p )
            return p;
        int iRight = isNext;
        int iLeft = isNext ? 1 : 0;
        if ( p->children[iRight] )
        { // go to left most leaf of right subtree
            p = p->children[iRight];
            while ( p->children[iLeft] )
                p = p->children[iLeft];
            return p;
        }
        else
        { // go up until it's a left child or it's the root.
            for ( ; p->parent; p = p->parent )
            {
                if ( p == p->parent->children[iLeft] )
                { // it's the left child of parent
                    return p->parent;
                } // else it's the right child of parent
            }
            // now p is root
            return nullptr;
        }
    }

    // Inorder iterator
    struct iterator
    {
        Node *node;
        iterator( Node *node = nullptr ) : node( node )
        {
        }
        value_type *operator->()
        {
            return node->kv;
        }
        value_type &operator*()
        {
            return node->kv;
        }
        iterator &operator++()
        {
            node = nextNode( node );
            return *this;
        }
        iterator operator++( int )
        {
            iterator it( node );
            node = nextNode( node );
            return it;
        }
        iterator &operator--()
        {
            node = nextNode( node, 0 );
            return *this;
        }
        iterator operator--( int )
        {
            iterator it( node );
            node = nextNode( node, 0 );
            return it;
        }
        iterator parent() const
        {
            return iterator( node ? node->parent : node );
        }
        iterator child( int idx ) const
        {
            return iterator( node ? node->children[idx] : node );
        }
        iterator left() const
        {
            return iterator( node ? node->children[ILEFT] : node );
        }
        iterator right() const
        {
            return iterator( node ? node->children[IRIGHT] : node );
        }
        bool hasNext() const
        {
            return nextNode( node );
        }
        bool hasPrevious() const
        {
            return nextNode( node, 0 );
        }
    };
};
} // namespace ftl
#endif
