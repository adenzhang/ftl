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

#ifndef BINARY_SEARCH_TREE_H
#define BINARY_SEARCH_TREE_H

#include <binary_tree.h>

namespace ftl
{

template<typename KeyType, typename ValueType, typename KeyCompare = std::less<KeyType>>
class binary_search_tree : public binary_tree<KeyType, ValueType>
{
    // private types
public:
    // public types
    typedef binary_tree<KeyType, ValueType> SuperType;
    typedef typename SuperType::Node Node;
    typedef typename SuperType::iterator iterator;
    typedef typename SuperType::value_type value_type;

    //	using BinaryTreeType<KeyType, ValueType>::Node;
    enum
    {
        ILEFT = 0,
        IRIGHT = 1,
        NUM_CHILDREN = 2
    };

protected:
    // private members
    Node *_root;
    size_t _size;
    KeyCompare _keyLess;

public:
    // public functions
    binary_search_tree( const KeyCompare &kc = std::less<KeyType>() ) : _root( nullptr ), _size( 0 ), _keyLess( kc )
    {
    }

    iterator root()
    {
        return iterator( _root );
    }
    iterator begin()
    {
        Node *p = _root;
        if ( p )
            while ( p->children[ILEFT] )
                p = p->children[ILEFT];
        return iterator( p );
    }
    iterator last()
    {
        Node *p = _root;
        if ( p )
            while ( p->children[IRIGHT] )
                p = p->children[IRIGHT];
        return iterator( p );
    }
    iterator end()
    {
        return iterator();
    }
    iterator lower_bound( const KeyType &v )
    {
        // return the first element that >= v
        Node *p = _root;
        if ( p && !_keyLess( p->kv.first, v ) )
        {
            while ( true )
            {
                Node *prev = nextNode( p, 0 );
                if ( prev && !_keyLess( prev->kv.first, v ) )
                {
                    p = prev;
                }
                else
                {
                    return iterator( p );
                }
            }
        }
        for ( ; p && _keyLess( p->kv.first, v ); p = nextNode( p ) )
            ;
        return iterator( p );
    }
    iterator upper_bound( const KeyType &v )
    {
        // return the first element that > v
        Node *p = _root;
        if ( p && _keyLess( v, p->kv.first ) )
        {
            while ( true )
            {
                Node *prev = nextNode( p, 0 );
                if ( prev && _keyLess( v, prev->kv.first ) )
                {
                    p = prev;
                }
                else
                {
                    return iterator( p );
                }
            }
        }
        for ( ; p && !_keyLess( v, p->kv.first ); p = nextNode( p ) )
            ;
        return iterator( p );
    }
    std::pair<iterator, bool> insert( const value_type &kv )
    {
        ++_size;
        if ( !_root )
        {
            _root = new Node( kv );
            return std::make_pair( _root, true );
        }
        Node *p = _root;
        while ( true )
        {
            if ( _keyLess( p->kv.first, kv.first ) )
            {
                if ( p->children[IRIGHT] )
                {
                    p = p->children[IRIGHT];
                }
                else
                {
                    p = SuperType::addChild( p, new Node( kv ), IRIGHT );
                    break;
                }
            }
            else if ( _keyLess( kv.first, p->kv.first ) )
            {
                if ( p->children[ILEFT] )
                {
                    p = p->children[ILEFT];
                }
                else
                {
                    p = SuperType::addChild( p, new Node( kv ), ILEFT );
                    break;
                }
            }
            else
            {
                --_size;
                return std::make_pair( iterator( p ), false );
            }
        }
        return std::make_pair( p, true );
    }
};

} // namespace ftl
#endif
