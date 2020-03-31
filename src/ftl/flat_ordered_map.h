#pragma once
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <map>


namespace ftl
{

template<class T1, class T2>
struct LessThan;

template<class T>
struct LessThan<T, T> : std::less<T>
{
};


template<class T1, class T2>
struct LessThan
{
    bool operator()( const T1 &x, const T2 &y ) const
    {
        return x < y;
    }
};

template<class T>
struct LessThanOther
{
    template<class U>
    bool operator()( const T &x, const U &y ) const
    {
        return x < y;
    }
};

struct Less
{
    template<class T1, class T2>
    bool operator()( const T1 &x, const T2 &y ) const
    {
        return x < y;
    }
};

struct Compare
{
    // if there's LessThan<T1, T2>
    template<class T1, class T2, class Less = LessThan<T1, T2>>
    int operator()( const T1 &x, const T2 &y, const Less &less ) const
    {
        return less( x, y ) ? ( -1 ) : ( less( y, x ) ? 1 : 0 );
    }

    template<class T1, class T2>
    int operator()( const T1 &x, const T2 &y ) const
    {
        return ( x < y ) ? ( -1 ) : ( ( y < x ) ? 1 : 0 );
    }
    // todo: if type T has compare(T2)
};
static constexpr Compare compare;

#define COMPARE_IF_NOT
template<class InputIterator1, class InputIterator2, class LessT = Less> // todo add LessThan
int lexico_compare( InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, const LessT &less = LessT{} )
{
    for ( ; first1 != last1; ++first1, ++first2 )
    {
        auto r = compare( *first1, *first2, less );
        if ( r != 0 )
            return r;
    }
    return ( first2 != last2 ) ? ( -1 ) : 0;
}

// compare collections
template<class T1, class T2, class LessT = Less>
int lexico_compare( const T1 &x, const T2 &y, const Less &less = LessT{} )
{
    return lexico_compare( x.begin(), x.end(), y.begin(), y.end(), less );
}


template<class K, class V>
using KVPair = std::conditional_t<std::is_same_v<V, void>, K, std::pair<K, V>>;

template<class K, class V, bool is_multi_map, class LessThan, class Vector>
class FlatOrderedMapBase : public Vector
{
public:
    using this_type = FlatOrderedMapBase;
    using base_type = Vector;

    static constexpr bool is_set = std::is_same_v<V, void>;
    using key_type = K;
    using mapped_type = std::conditional_t<is_set, K, V>;
    using allocator_type = typename base_type::allocator_type;

    using value_type = typename base_type::value_type;
    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;

    using base_type::back;
    using base_type::begin;
    using base_type::cbegin;
    using base_type::cend;
    using base_type::empty;
    using base_type::end;
    using base_type::front;
    using base_type::rbegin;
    using base_type::rend;
    using base_type::size;

    using base_type::clear;
    using base_type::get_allocator;
    using base_type::reserve;
    using base_type::resize;
    using base_type::swap;

    FlatOrderedMapBase( size_t n, const value_type &v, const LessThan &less = LessThan{}, const allocator_type &alloc = allocator_type{} )
        : base_type( n, v, alloc ), m_less( less )
    {
    }

    FlatOrderedMapBase( const LessThan &less = LessThan{}, const allocator_type &alloc = allocator_type{} ) : base_type( alloc ), m_less( less )
    {
    }

    template<class InputIter>
    FlatOrderedMapBase( InputIter it, InputIter itEnd, const LessThan &less = LessThan{}, const allocator_type &alloc = allocator_type{} )
        : base_type( it, itEnd, alloc ), m_less( less )
    {
        std::sort( base_type::begin(), base_type::end() );
    }

    FlatOrderedMapBase( const std::initializer_list<value_type> &il ) : FlatOrderedMapBase( il.begin(), il.end() )
    {
    }

    FlatOrderedMapBase( const FlatOrderedMapBase &a ) : base_type( a ), m_less( a.m_less )
    {
    }
    FlatOrderedMapBase( FlatOrderedMapBase &&a ) : base_type( std::move( a ) ), m_less( std::move( a.m_less ) )
    {
    }
    this_type &operator=( this_type &&a )
    {
        base_type::operator=( std::move( a ) );
        m_less = std::move( a.m_less );
        return *this;
    }

    this_type &operator=( const this_type &a )
    {
        base_type::operator=( a );
        m_less = a.m_less;
        return *this;
    }

    void update( const this_type &a, bool bInsertIfNotFound = true )
    {
        for ( const auto &p : a )
            update( p, bInsertIfNotFound );
    }
    void update( this_type &&a, bool bInsertIfNotFound = true )
    {
        for ( auto &&p : a )
            update( std::move( p ), bInsertIfNotFound );
    }

    this_type &operator+=( const this_type &a )
    {
        update( a );
        return *this;
    }

    this_type &operator+=( this_type &&a )
    {
        update( std::move( a ) );
        return *this;
    }

    this_type &operator+=( const value_type &a )
    {
        update( a );
        return *this;
    }

    this_type &operator+=( value_type &&a )
    {
        update( std::move( a ) );
        return *this;
    }

    void erase( const K &k )
    {
        auto p = equal_range( k );
        base_type::erase( p.first, p.second );
    }

    template<class KT>
    const_iterator lower_bound( const KT &k ) const
    {
        return std::lower_bound( base_type::begin(), base_type::end(), k, m_less );
    }
    template<class KT>
    iterator lower_bound( const KT &k )
    {
        return std::lower_bound( base_type::begin(), base_type::end(), k, m_less );
    }

    template<class KT>
    const_iterator upper_bound( const KT &k ) const
    {
        return std::upper_bound( base_type::begin(), base_type::end(), k, m_less );
    }
    template<class KT>
    iterator upper_bound( const KT &k )
    {
        return std::upper_bound( base_type::begin(), base_type::end(), k, m_less );
    }

    template<class KT>
    const_iterator find( const KT &k ) const
    {
        auto it = lower_bound( k );
        return ( it != base_type::end() && !m_less( k, *it ) ) ? it : base_type::end();
    }
    template<class KT>
    iterator find( const KT &k )
    {
        auto it = lower_bound( k );
        return ( it != base_type::end() && !m_less( k, *it ) ) ? it : base_type::end();
    }

    template<class KT>
    std::pair<const_iterator, const_iterator> equal_range( const KT &k ) const
    {
        auto it = find( k );
        if ( it == base_type::end() )
            return {};
        if constexpr ( is_multi_map )
            return {it, upper_bound( k )};
        else
            return {};
    }
    template<class KT>
    std::pair<iterator, iterator> equal_range( const KT &k )
    {
        auto it = find( k );
        if ( it == base_type::end() )
            return {};
        if constexpr ( is_multi_map )
            return {it, upper_bound( k )};
        else
            return {it, std::next( it, 1 )};
    }
    size_t conut( const K &k ) const
    {
        auto p = equal_range( k );
        return std::distance( p.first, p.second );
    }

    // return <iterator points to val, Inserted>.
    // if is multi map, always insert.
    std::pair<iterator, bool> update( const value_type &val, bool bInsertIfNotFound = true )
    {
        auto it = lower_bound( get_key( val ) );
        if constexpr ( is_multi_map )
        {
            return {*base_type::insert( it, val ), true};
        }
        else
        {
            if ( it != base_type::end() && !m_less( val, *it ) )
            {
                if constexpr ( !is_set )
                    get_mapped( *it ) = val;
                return {it, false};
            }
            if ( bInsertIfNotFound )
                return {*base_type::insert( it, val ), true};
            return {base_type::end(), false};
        }
    }

    std::pair<iterator, bool> update( value_type &&val, bool bInsertIfNotFound = true )
    {
        auto it = lower_bound( get_key( val ) );
        if constexpr ( is_multi_map )
        {
            return {*base_type::insert( it, std::move( val ) ), true};
        }
        else
        {
            if ( it != base_type::end() && !m_less( val, *it ) )
            {
                if constexpr ( !is_set )
                    get_mapped( *it ) = std::move( get_mapped( val ) );
                return {it, false};
            }
            if ( bInsertIfNotFound )
                return {base_type::insert( it, std::move( val ) ), true};
            return {base_type::end(), false};
        }
    }

    std::pair<iterator, bool> insert( const value_type &val )
    {
        auto it = lower_bound( get_key( val ) );
        if constexpr ( is_multi_map )
        {
            return {base_type::insert( it, val ), true};
        }
        else
        {
            if ( it != base_type::end() && !m_less( val, *it ) )
                return {iterator{}, false};
            else
                return {base_type::insert( it, val ), true};
        }
    }

    template<bool IsMap = !is_set, typename = std::enable_if_t<IsMap, void>>
    mapped_type &operator[]( const key_type &k )
    {
        return get_mapped( *update( value_type{k, mapped_type{}}, true ).first );
    }

    // throws std::out_of_range
    template<bool IsMap = !is_set, typename = std::enable_if_t<IsMap, void>>
    const mapped_type &operator[]( const key_type &k ) const
    {
        auto it = lower_bound( k );
        if ( it != base_type::end() && !m_less( k, *it ) )
            return get_mapped( *it );

        throw std::out_of_range( "No key vale" );
    }

    template<class Iter>
    int compare_sorted( Iter it, Iter itEnd )
    {
        return lexico_compare( base_type::begin(), base_type::end(), it, itEnd );
    }
    template<class AnyCollection>
    int compare( const AnyCollection &a )
    {
        return compare_sorted( a.begin(), a.end() );
    }
    template<class AnyCollection>
    bool operator==( const AnyCollection &a ) const
    {
        return compare( a ) == 0;
    }

    template<class AnyCollection>
    bool operator<( const AnyCollection &a ) const
    {
        return compare( a ) < 0;
    }

protected:
    static constexpr const K &get_key( const value_type &kv )
    {
        if constexpr ( is_set )
        {
            return kv;
        }
        else
        {
            return kv.first;
        }
    }
    static constexpr mapped_type &get_mapped( value_type &kv )
    {
        if constexpr ( is_set )
        {
            return kv;
        }
        else
        {
            return kv.second;
        }
    }

    struct KeyLess
    {
        bool operator()( const value_type &kv1, const value_type &kv2 ) const
        {
            return m_less( get_key( kv1 ), get_key( kv2 ) );
        }

        template<class KT>
        bool operator()( const value_type &kv1, const KT &k ) const
        {
            return m_less( get_key( kv1 ), k );
        }
        template<class KT>
        bool operator()( const KT &k, const value_type &kv1 ) const
        {
            return m_less( k, get_key( kv1 ) );
        }
        template<class KT1, class KT2>
        bool operator()( const KT1 &k1, const KT2 &k2 ) const
        {
            return m_less( k1, k2 );
        }

        KeyLess( const LessThan &lt = LessThan{} ) : m_less( lt )
        {
        }
        LessThan m_less;
    };
    KeyLess m_less;
};

template<class K,
         class V,
         class LessThan = LessThanOther<K>,
         class Allocator = std::allocator<KVPair<K, V>>,
         class Vector = std::vector<KVPair<K, V>, Allocator>>
using FlatOrderedMap = FlatOrderedMapBase<K, V, false, LessThan, Vector>;

template<class K,
         class V,
         class LessThan = std::less<K>,
         class Allocator = std::allocator<KVPair<K, V>>,
         class Vector = std::vector<KVPair<K, V>, Allocator>>
using FlatOrderedMultiMap = FlatOrderedMapBase<K, V, true, LessThan, Vector>;

template<class K,
         class LessThan = std::less<K>,
         class Allocator = std::allocator<KVPair<K, void>>,
         class Vector = std::vector<KVPair<K, void>, Allocator>>
using FlatOrderedSet = FlatOrderedMapBase<K, void, false, LessThan, Vector>;

template<class K,
         class LessThan = std::less<K>,
         class Allocator = std::allocator<KVPair<K, void>>,
         class Vector = std::vector<KVPair<K, void>, Allocator>>
using FlatOrderedMultiSet = FlatOrderedMapBase<K, void, true, LessThan, Vector>;

} // namespace ftl
