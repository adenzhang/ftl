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


namespace ftl
{

template<typename ValueType, typename AccType = std::plus<ValueType>, typename ContainerType = std::vector<ValueType>>
class indexed_tree
{
public:
    // public types
    typedef ValueType value_type;
    typedef AccType AccFunc;
    //    typedef std::binary_function<const ValueType&, const ValueType&, ValueType> AccFunc;
protected:
    AccFunc accFunc;
    ContainerType biTree;

public:
    indexed_tree( size_t size = 0, const AccFunc &func = AccType() ) : accFunc( func ), biTree( size + 1 )
    {
    }

    indexed_tree( typename ContainerType::iterator it, typename ContainerType::iterator itEnd, const AccFunc &func = AccType() ) : accFunc( func )
    {
        construct( it, itEnd );
    }

    indexed_tree( typename ContainerType::iterator it, size_t N, const AccFunc &func = AccType() ) : accFunc( func )
    {
        construct( it, N );
    }

    void resize( size_t size )
    {
        biTree.resize( size + 1 );
    }
    void add( size_t pos, const ValueType &v )
    {
        // index in BITree[] is 1 more than the index in arr[]
        ++pos;
        const size_t N = biTree.size();
        while ( pos <= N )
        {
            biTree[pos] = accFunc( v, biTree[pos] );
            pos += pos & ( -pos ); // add the last digit 1 to get higher level position.
        }
    }

    template<typename Iterator>
    void construct( Iterator it, const size_t N = 0 )
    {
        resize( N );
        for ( size_t i = 0; i < N; ++i, ++it )
        {
            add( i, *it );
        }
    }

    template<typename Iterator>
    void construct( Iterator it, Iterator itEnd )
    {
        resize( std::distance( it, itEnd ) );
        for ( size_t i = 0; it != itEnd; ++it, ++i )
        {
            add( i, *it );
        }
    }

    // Returns sum of arr[0..index]. This function assumes
    // that the array is preprocessed and partial sums of
    // array elements are stored in BITree[].
    ValueType getResult( size_t n ) const
    {
        ValueType sum;
        ++n;
        while ( n > 0 )
        {
            sum = accFunc( biTree[n], sum );
            n -= n & ( -n ); // remove last bit 1 to get parent position.
        }
        return sum;
    }

    ContainerType &getBiTree()
    {
        return biTree;
    }
};


} // namespace ftl


#endif //
