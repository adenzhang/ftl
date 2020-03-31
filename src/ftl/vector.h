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

#pragma once
#include <type_traits>
#include <memory>
#include <cstring>

/// @brief Generic VectorBase<T, BuferSize, Alloc, bool HasSizeVar, bool HasNullValue>.
/// It can be used as inplace vector/string, small-buffer-optimized vector/string, and dynamic allocated vector/string.
/// See Vector, Array (inplace array), InplaceCStr (inplace null terminated c string), CharCStr.
///


namespace ftl
{


// destroy elements in [pbeing, middle), and move [middle, pend) to begin.
template<class T>
void destroy( T *pbegin, T *middle, T *pend )
{
    using Distance = std::ptrdiff_t;
    if ( pbegin != middle )
    {
        if constexpr ( std::is_trivially_copyable_v<T> )
        {
            // for(; middle != pend; ++pbegin, ++middle)
            //     *pbegin = *middle;
            std::memmove( pbegin, middle, sizeof( T ) * ( pend - middle ) );
        }
        else if constexpr ( std::is_move_assignable_v<T> )
        {
            for ( Distance n = pend - middle; n > 0; --n, ++pbegin, ++middle )
                *pbegin = std::move( middle );
            for ( std::ptrdiff_t n = pend - pbegin; n > 0; --n, ++pbegin )
                pbegin->~T();
        }
        else if constexpr ( std::is_copy_assignable_v<T> )
        {
            for ( Distance n = pend - middle; n > 0; --n, ++pbegin, ++middle )
                *pbegin = middle;
            for ( Distance n = pend - pbegin; n > 0; --n, ++pbegin )
                pbegin->~T();
        }
        else
        {
            for ( auto p = pbegin; p != middle; ++p )
                p->~T();
            for ( Distance n = pend - middle; n > 0; --n, ++pbegin, ++middle )
            {
                if constexpr ( std::is_move_constructible_v<T> )
                    new ( pbegin ) T( std::move( *middle ) );
                else
                    new ( pbegin ) T( *middle );
                middle->~T();
            }
        }
    }
}


template<class T>
void assign_or_construct( T &lhs, T &rhs )
{
    if constexpr ( std::is_move_assignable_v<T> )
        lhs = std::move( rhs );
    else if constexpr ( std::is_copy_assignable_v<T> )
        lhs = rhs;
    else if constexpr ( std::is_move_constructible_v<T> )
    {
        lhs.~T();
        new ( &lhs ) T( std::move( rhs ) );
    }
    else
    {
        lhs.~T();
        new ( &lhs ) T( rhs );
    }
}

template<class T, class F>
T *remove_if( T *pbegin, T *pend, F &&pred )
{
    using Distance = std::ptrdiff_t;
    auto result = pbegin;
    for ( Distance n = pend - pbegin; n > 0; --n, ++pbegin )
    {
        if ( !pred( *pbegin ) ) // keep it
        {
            if ( result != pbegin )
            {
                assign_or_construct( *result, *pbegin );
            }
            ++result;
        } // else remove it
    }
    return result;
}

/// @class arrayview View of buffer.
template<typename T, bool HasCapacity = true>
struct arrayview;

template<typename T>
struct arrayview<T, false>
{
    static constexpr bool has_capacity = false;

    using this_type = arrayview<T, false>;
    using value_type = T;
    using iterator = T *;
    using const_iterator = T *;

    arrayview( T *data, size_t n = 0 ) : m_data( data ), m_size( n )
    {
    }
    arrayview() = default;
    arrayview( const arrayview & ) = default;
    this_type &operator=( const arrayview & ) = default;

    size_t size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return m_size == 0;
    }

    iterator begin()
    {
        return m_data;
    }
    iterator end()
    {
        return m_data + m_size;
    }
    const_iterator cbegin() const
    {
        return m_data;
    }
    const_iterator cend() const
    {
        return m_data + m_size;
    }
    const_iterator begin() const
    {
        return m_data;
    }
    const_iterator end() const
    {
        return m_data + m_size;
    }
    T &operator[]( size_t n )
    {
        return m_data[n];
    }
    const T &operator[]( size_t n ) const
    {
        return m_data[n];
    }

    this_type sub( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        if ( pos >= m_size )
            return {};
        maxlen = std::min( maxlen, m_size - pos );
        return this_type( m_data + pos, maxlen );
    }

protected:
    T *m_data = nullptr;
    size_t m_size = 0;
};

template<typename T>
struct arrayview<T, true> : public arrayview<T, false>
{
    static constexpr bool has_capacity = true;

    using this_type = arrayview<T, true>;
    using base_type = arrayview<T, false>;

    using base_type::begin;
    using base_type::cbegin;
    using base_type::cend;
    using base_type::end;
    using base_type::size;
    using base_type::operator[];
    using base_type::empty;

    arrayview( T *d, size_t n, size_t c ) : base_type( d, n ), m_cap( std::max( n, c ) )
    {
    }

    arrayview( const arrayview<T, false> &a ) : base_type( a ), m_cap( a.size() )
    {
    }
    arrayview() = default;
    arrayview( const arrayview & ) = default;
    this_type &operator=( const arrayview & ) = default;


    size_t capacity() const
    {
        return m_cap;
    }
    template<class... Args>
    T *emplace_back( Args &&... args )
    {
        if ( base_type::m_size < m_cap )
        {
            new ( base_type::m_data + base_type::m_size ) T( std::forward<Args>( args )... );
            return base_type::m_data + ( base_type::m_size++ );
        }
        return nullptr;
    }

    bool pop_back()
    {
        if ( base_type::m_size )
        {
            base_type::m_data[--base_type::m_size].~T();
            return true;
        }
        return false;
    }

    void clear()
    {
        while ( size() )
            pop_back();
    }

    this_type sub( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        if ( pos >= base_type::m_size )
            return {};
        maxlen = std::min( maxlen, base_type::m_size - pos );
        return this_type( base_type::m_data + pos, maxlen, m_cap - pos );
    }

protected:
    size_t m_cap;
};

template<class T>
arrayview<T, false> make_view( T *p, size_t n )
{
    return {p, n};
}
template<class T>
arrayview<T, true> make_view( T *p, size_t n, size_t cap )
{
    return {p, n, cap};
}
inline arrayview<const char, false> make_view( const char *p )
{
    return {p, std::strlen( p )};
}

/// T: element type.
/// N: inplace buffer size.
template<class Impl,
         class T,
         size_t N,
         class Alloc = std::allocator<T>,
         bool HasSizeVar = true,
         bool HasNullValue = false,
         class NullType = std::conditional_t<HasNullValue, T, int>,
         NullType NullValue = NullType{},
         typename EnableDynamicAlloc = void>
struct SboStorage;

// Alloc is void, HasSizeVar
template<class Impl, class T, size_t N, class Alloc, bool HasSizeVar, bool HasNullValue, class NullType, NullType NullValue>
struct SboStorage<Impl, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue, std::enable_if_t<HasSizeVar && std::is_same_v<Alloc, void>>>
{
    using this_type = SboStorage<Impl, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;

    static_assert( N > 0 );

    SboStorage() = default;

    void destroy()
    {
    }

    T *begin()
    {
        return reinterpret_cast<T *>( &m_data[0] );
    }
    constexpr size_t capacity() const
    {
        return HasNullValue ? ( N - 1 ) : N;
        return N;
    }
    constexpr size_t buffer_size() const
    {
        return N;
    }
    size_t size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return !m_size;
    }

    bool reserve( size_t n )
    {
        return n <= capacity();
    }

    void set_size( size_t n )
    {
        m_size = n;
        if constexpr ( HasNullValue )
            m_data[n] = NullValue;
    }
    constexpr bool using_inplace() const
    {
        return true;
    }

private:
    size_t m_size = 0;
    typename std::aligned_storage<sizeof( T ), alignof( T )>::type m_data[N];
};

// Alloc is void, no size var, has null value.
template<class Impl, class T, size_t N, class Alloc, bool HasSizeVar, bool HasNullValue, class NullType, NullType NullValue>
struct SboStorage<Impl,
                  T,
                  N,
                  Alloc,
                  HasSizeVar,
                  HasNullValue,
                  NullType,
                  NullValue,
                  std::enable_if_t<HasNullValue && HasSizeVar && std::is_same_v<Alloc, void>>>
{
    using this_type = SboStorage<Impl, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;

    static_assert( N > 0 );

    SboStorage() = default;

    void destroy()
    {
    }

    T *begin()
    {
        return reinterpret_cast<T *>( &m_data[0] );
    }
    constexpr size_t capacity() const
    {
        return HasNullValue ? ( N - 1 ) : N;
        return N;
    }
    constexpr size_t buffer_size() const
    {
        return N;
    }
    size_t size() const
    {
        size_t i = 0;
        for ( auto p = begin(); *p != NullValue; ++p, ++i )
            ;
        return i;
    }
    bool empty() const
    {
        return m_data[0] == NullValue;
    }

    bool reserve( size_t n )
    {
        return n <= capacity();
    }
    void set_size( size_t n )
    {
        m_data[n] = NullValue;
    }
    constexpr bool using_inplace() const
    {
        return true;
    }

private:
    typename std::aligned_storage<sizeof( T ), alignof( T )>::type m_data[N];
};


// Non-void Alloc, has size var. has inplace buffer
template<class Impl, class T, size_t N, class Alloc, bool HasSizeVar, bool HasNullValue, class NullType, NullType NullValue>
struct SboStorage<Impl,
                  T,
                  N,
                  Alloc,
                  HasSizeVar,
                  HasNullValue,
                  NullType,
                  NullValue,
                  std::enable_if_t<N != 0 && HasSizeVar && !std::is_same_v<Alloc, void>>>
{
    using this_type = SboStorage<Impl, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;
    using size_type = size_t;

    SboStorage( const Alloc &alloc ) : m_alloc( alloc ), m_begin( reinterpret_cast<T *>( &m_data[0] ) )
    {
    }
    SboStorage( Alloc &&alloc = Alloc{} ) : m_alloc( std::move( alloc ) ), m_begin( reinterpret_cast<T *>( &m_data[0] ) )
    {
    }

    void destroy()
    {
        if ( !using_inplace() && m_begin )
            m_alloc.deallocate( m_begin, m_bufferSize );
    }

    T *begin()
    {
        return m_begin;
    }
    size_t buffer_size() const
    {
        return m_bufferSize;
    }
    size_t capacity() const
    {
        return HasNullValue ? ( m_bufferSize - 1 ) : m_bufferSize;
    }
    size_t size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return !m_size;
    }

    bool reserve( size_t n )
    {
        if ( n <= capacity() )
            return true;
        auto newCap = HasNullValue ? ( n + 1 ) : n;
        if ( m_size == capacity() )
            newCap = std::max( 2 * m_bufferSize, newCap ); // double capacity
        auto pBuf = m_alloc.allocate( newCap );
        if ( !pBuf )
            return false;
        auto pSrc = m_begin, pDest = pBuf;
        for ( size_t i = 0; i < m_size; ++i, ++pSrc, ++pDest )
        {
            new ( pDest ) T( std::move( *pSrc ) );
            pSrc->~T();
        }
        if constexpr ( HasNullValue )
            *pDest = NullValue;
        if ( !using_inplace() )
            m_alloc.deallocate( m_begin, m_bufferSize );
        m_begin = pBuf;
        m_bufferSize = newCap;
        return true;
    }

    bool using_inplace() const
    {
        return m_begin == reinterpret_cast<const T *>( &m_data[0] );
    }

    void set_size( size_t n )
    {
        m_size = n;
        if constexpr ( HasNullValue )
            m_begin[n] = NullValue;
    }

    Alloc &get_allocator()
    {
        return m_alloc;
    }
    const Alloc &get_allocator() const
    {
        return m_alloc;
    }

    arrayview<T> try_moveout()
    {
        if ( using_inplace() )
            return {};
        arrayview<T> res{m_begin, m_size, m_bufferSize};
        m_begin = reinterpret_cast<T *>( &m_data[0] );
        m_size = 0;
        m_bufferSize = N;
        return res;
    }

    // user must check capacity before calling this function
    void movein( arrayview<T> &s )
    {
        static_cast<Impl *>( this )->~Impl();
        m_begin = s.begin();
        m_size = s.size();
        m_bufferSize = s.capacity();
        if constexpr ( HasNullValue )
            m_begin[m_size] = NullValue;
    }

private:
    Alloc m_alloc;
    T *m_begin = nullptr;
    size_type m_size = 0;
    size_type m_bufferSize = N;
    typename std::aligned_storage<sizeof( T ), alignof( T )>::type m_data[N];
};

// Non-void Alloc, has size var. No inplace buffer
template<class Impl, class T, size_t N, class Alloc, bool HasSizeVar, bool HasNullValue, class NullType, NullType NullValue>
struct SboStorage<Impl,
                  T,
                  N,
                  Alloc,
                  HasSizeVar,
                  HasNullValue,
                  NullType,
                  NullValue,
                  std::enable_if_t<N == 0 && HasSizeVar && !std::is_same_v<Alloc, void>>>
{
    using this_type = SboStorage<Impl, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;

    using size_type = size_t;

    SboStorage( const Alloc &alloc ) : m_alloc( alloc )
    {
    }
    SboStorage( Alloc &&alloc = Alloc{} ) : m_alloc( std::move( alloc ) )
    {
    }

    void destroy()
    {
        if ( m_begin )
            m_alloc.deallocate( m_begin, m_bufferSize );
    }

    T *begin()
    {
        return reinterpret_cast<T *>( m_begin );
    }
    size_type size() const
    {
        return m_size;
    }
    size_type buffer_size() const
    {
        return m_bufferSize;
    }
    size_type capacity() const
    {
        return HasNullValue ? ( m_bufferSize - 1 ) : m_bufferSize;
    }

    bool reserve( size_t n )
    {
        if ( n <= capacity() )
            return true;
        auto newCap = HasNullValue ? ( n + 1 ) : n;
        if ( m_size == capacity() )
            newCap = std::max( 2 * m_bufferSize, newCap ); // double capacity
        auto pBuf = m_alloc.allocate( newCap );
        if ( !pBuf )
            return false;
        auto pSrc = m_begin, pDest = pBuf;
        for ( size_t i = 0; i < m_size; ++i, ++pSrc, ++pDest )
        {
            new ( pDest ) T( std::move( *pSrc ) );
            pSrc->~T();
        }
        if constexpr ( HasNullValue )
            *pDest = NullValue;
        if ( !using_inplace() )
            m_alloc.deallocate( m_begin, m_bufferSize );
        m_begin = pBuf;
        m_bufferSize = newCap;
        return true;
    }

    constexpr bool using_inplace() const
    {
        return false;
    }

    void set_size( size_t n )
    {
        m_size = n;
        if constexpr ( HasNullValue )
            m_begin[n] = NullValue;
    }

    Alloc &get_allocator()
    {
        return m_alloc;
    }
    const Alloc &get_allocator() const
    {
        return m_alloc;
    }

    arrayview<T> try_moveout()
    {
        arrayview<T> res{m_begin, m_size, m_bufferSize};
        m_begin = nullptr;
        m_size = 0;
        m_bufferSize = 0;
        return res;
    }

    void movein( arrayview<T> &s )
    // user must check capacity before calling this function
    {
        static_cast<Impl *>( this )->~Impl();
        m_begin = s.begin();
        m_size = s.size();
        m_bufferSize = s.capacity();
        if constexpr ( HasNullValue )
            m_begin[m_size] = NullValue;
    }

private:
    Alloc m_alloc;
    T *m_begin = nullptr;
    size_type m_size = 0;
    size_type m_bufferSize = 0;
};

template<class T,
         size_t N = 0,
         class Alloc = std::allocator<T>,
         bool HasSizeVar = true,
         bool HasNullValue = false,
         class NullType = std::conditional_t<HasNullValue, T, int>,
         NullType NullValue = NullType{}>
class VectorBase : public SboStorage<VectorBase<T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>,
                                     T,
                                     N,
                                     Alloc,
                                     HasSizeVar,
                                     HasNullValue,
                                     NullType,
                                     NullValue>
{
public:
    using this_type = VectorBase<T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;
    using base_type = SboStorage<this_type, T, N, Alloc, HasSizeVar, HasNullValue, NullType, NullValue>;

    using value_type = T;
    using pointer_type = T *;
    using reference_type = T &;
    using const_reference_type = const T &;

    using base_type::begin;
    using base_type::buffer_size;
    using base_type::capacity;
    using base_type::reserve;
    using base_type::size;

    using iterator = pointer_type;
    using const_iterator = const T *;

    static constexpr size_t inplace_buffer_size = N;
    static constexpr bool is_string = HasNullValue;
    static constexpr size_t inplace_capacity = is_string ? ( N > 0 ? ( N - 1 ) : 0 ) : N;
    static constexpr bool has_allocator = !std::is_same_v<Alloc, void>;

    static constexpr auto npos = std::numeric_limits<size_t>::max();

    VectorBase() = default;

    template<class Iter>
    VectorBase( Iter it, Iter itEnd )
    {
        push_back_iter( it, itEnd );
    }

    template<class A, typename = std::enable_if_t<std::is_same_v<A, Alloc> && !std::is_same_v<A, void>, void>>
    VectorBase( const A &alloc ) : base_type( alloc )
    {
    }
    template<class U>
    VectorBase( const std::initializer_list<U> &il )
    {
        push_back( il );
    }

    // todo: requires is_constructible<T, *Iter>
    template<class Iter, typename std::enable_if_t<HasNullValue && std::is_same_v<const decltype( *std::declval<Iter>() ), const T &>>>
    VectorBase( Iter it )
    {
        push_back_iter( it );
    }

    VectorBase( size_t n, const T &v = T{} )
    {
        push_back( v, n );
    }

    template<typename U, size_t M>
    VectorBase( U ( &a )[M] )
    {
        push_back_iter( &a[0], M );
    }

    // has alloc
    //    template<class U,
    //             size_t M,
    //             class A,
    //             bool bSize,
    //             bool bNull,
    //             class NullT,
    //             NullT NullV,
    //             typename std::enable_if_t<std::is_same_v<A, Alloc> && !std::is_same_v<void, A>, void>>
    //    VectorBase( const VectorBase<U, M, A, bSize, bNull, NullT, NullV> &a ) : base_type( a.get_allocator() )
    //    {
    //        push_back_iter( a.begin(), a.end() );
    //    }

    // both have same no-avoid alloc. potentially move out
    template<class U,
             size_t M,
             class A,
             bool bSize,
             bool bNull,
             class NullT,
             NullT NullV,
             typename = std::enable_if_t<std::is_same_v<A, Alloc> && !std::is_same_v<void, A>, void>>
    VectorBase( VectorBase<U, M, A, bSize, bNull, NullT, NullV> &&a ) : base_type( std::move( a.get_allocator() ) )
    {
        if ( !a.using_inplace() && ( !is_string || a.buffer_size() > a.size() ) )
        {
            auto slice = a.try_moveout();
            base_type::movein( slice );
        }
        else
        {
            push_back_moveiter( a.begin(), a.end() );
        }
    }

    // no alloc or not same alloc
    template<class U, size_t M, class A, bool bSize, bool bNull, class NullT, NullT NullV>
    VectorBase( const VectorBase<U, M, A, bSize, bNull, NullT, NullV> &a )
    {
        push_back_iter( a.begin(), a.end() );
    }

    ~VectorBase()
    {
        clear();
        base_type::destroy();
    }

    // has alloc
    template<class U,
             size_t M,
             class A,
             bool bSize,
             bool bNull,
             class NullT,
             NullT NullV,
             typename = std::enable_if_t<std::is_same_v<A, Alloc> && !std::is_same_v<void, A>, void>>
    this_type &operator=( const VectorBase<U, M, A, bSize, bNull, NullT, NullV> &a )
    {
        this->~this_type();
        new ( this ) this_type( a );
        return *this;
    }

    // has alloc. move out
    template<class U,
             size_t M,
             class A,
             bool bSize,
             bool bNull,
             class NullT,
             NullT NullV,
             typename = std::enable_if_t<std::is_same_v<A, Alloc> && !std::is_same_v<void, A>, void>>
    this_type &operator=( VectorBase<U, M, A, bSize, bNull, NullT, NullV> &&a )
    {
        this->~this_type();
        new ( this ) this_type( std::move( a ) );
        return *this;
    }

    /// @note may fail if it's an inplace vector/string.
    this_type &operator+=( const T &v )
    {
        push_back( v );
        return *this;
    }

    template<class U, size_t M, class A, bool bSize, bool bNull, class NullT, T NullV>
    this_type &operator+=( const VectorBase<U, M, A, bSize, bNull, NullT, NullV> &a )
    {
        push_back_iter( a.begin().a.end() );
        return *this;
    }

    template<class U, size_t M, class A, bool bSize, bool bNull, class NullT, T NullV>
    this_type &operator+=( VectorBase<U, M, A, bSize, bNull, NullT, NullV> &&a )
    {
        push_back_moveiter( a.begin().a.end() );
        return *this;
    }

    template<class U, size_t M>
    this_type &operator+=( U ( &a )[M] )
    {
        push_back_iter( &a[0], M );
        return *this;
    }

    template<bool isString = HasNullValue>
    this_type substr( size_t pos = 0, size_t n = npos ) const
    {
        return sub( pos, n );
    }

    template<bool isString = HasNullValue>
    const_iterator c_str() const
    {
        return begin();
    }

    iterator end()
    {
        return begin() + size();
    }
    const_iterator cbegin() const
    {
        return const_cast<this_type *>( this )->begin();
    }
    const_iterator cend() const
    {
        return const_cast<this_type *>( this )->end();
    }
    const_iterator begin() const
    {
        return cbegin();
    }
    const_iterator end() const
    {
        return cend();
    }
    const T &operator[]( size_t n ) const
    {
        return *( begin() + n );
    }
    T &operator[]( size_t n )
    {
        return *( begin() + n );
    }
    const T &front() const
    {
        return *begin();
    }
    T &front()
    {
        return *begin();
    }
    const T &back() const
    {
        return *( begin() + size - 1 );
    }
    T &back()
    {
        return *( begin() + size() - 1 );
    }

    const arrayview<const T> sub_view( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        auto nsize = size();
        if ( pos >= nsize )
            return {};
        maxlen = std::min( maxlen, nsize - pos );
        return {begin() + pos, maxlen, buffer_size()};
    }

    this_type sub( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        auto nsize = size();
        if ( pos >= nsize )
            return {};
        maxlen = std::min( maxlen, nsize - pos );
        return this_type( begin() + pos, begin() + pos + maxlen );
    }

    template<class... Args>
    T *emplace_back( Args &&... args )
    {
        auto n = size();
        if ( !reserve( n + 1 ) )
            return nullptr;
        auto pElem = base_type::begin() + n;
        new ( pElem ) T( std::forward<Args>( args )... );
        set_size( n + 1 );
        return pElem;
    }

    /// @return size of resulting vector, or npos for failure.
    size_t pop_back( size_t n = 1 )
    {
        auto nsize = size();
        if ( nsize < n )
            return npos;
        auto pbegin = base_type::begin();
        for ( size_t i = 0; i < n; ++i )
        {
            auto pElem = pbegin + nsize - i - 1;
            pElem->~T();
        }
        base_type::set_size( nsize - n );
        return nsize - n;
    }

    /// If elememts are dynamically allocated, shrink the capacity to std::max(size(), minReserve).
    void shrink_to_fit( size_t minReserve = 0 )
    {
        if constexpr ( has_allocator )
        {
            auto nReserve = std::max( size(), minReserve );
            if ( !base_type::using_inplace() && capacity() > nReserve )
            {
                auto view = base_type::try_moveout();
                this->~this_type();
                new ( this ) this_type();
                reserve( nReserve );
                push_back_moveiter( view.begin(), view.end() );

                view.clear();
                base_type::get_allocator().deallocate( view.begin(), view.capacity() );
            }
        }
    }

    void clear()
    {
        auto nsize = size();
        auto pbegin = base_type::begin();
        for ( size_t i = 0; i < nsize; ++i )
        {
            auto pElem = pbegin + nsize - i - 1;
            pElem->~T();
        }
        base_type::set_size( 0 );
    }

    /// @return size of resulting vector, or npos for failure.
    size_t push_back( const T &v, size_t n = 1 )
    {
        auto nsize = size();
        if ( !reserve( nsize + n ) )
            return npos;
        auto pbegin = base_type::begin();
        for ( size_t i = 0; i < n; ++i )
        {
            new ( pbegin + nsize + i ) T( v );
        }
        base_type::set_size( nsize + n );
        return nsize + n;
    }

    bool resize( size_t n )
    {
        auto nsize = size();
        if ( n > nsize )
        {
            return push_back( T{}, n - nsize ) == n;
        }
        else if ( n < nsize )
        {
            return pop_back( nsize - n ) == n;
        }
        return true;
    }

    /// @return size of resulting vector, or npos for failure.
    template<class U>
    size_t push_back( const std::initializer_list<U> &il )
    {
        auto nsize = size();
        auto n = il.size();
        if ( !reserve( nsize + n ) )
            return npos;
        auto pbegin = base_type::begin();
        size_t i = 0;
        for ( auto &v : il )
            new ( pbegin + nsize + ( i++ ) ) T( v );
        base_type::set_size( nsize + n );
        return nsize + n;
    }

    template<class Iter>
    size_t push_back_iter( Iter it, Iter itEnd )
    {
        size_t n = std::distance( it, itEnd );
        auto nsize = size();
        if ( !reserve( nsize + n ) )
            return npos;
        auto pbegin = base_type::begin();
        for ( size_t i = 0; i < n; ++i, ++it )
        {
            new ( pbegin + nsize + i ) T( *it );
        }
        base_type::set_size( nsize + n );
        return nsize + n;
    }

    template<class Iter>
    size_t push_back_iter( Iter it, size_t maxlen )
    {
        if constexpr ( is_string )
        {
            auto n = size();
            for ( size_t i = 0; *it != NullValue && i < maxlen; ++n, ++it, ++i )
            {
                if ( !reserve( n + 1 ) )
                    break;
                new ( begin() + n ) T( *it );
                base_type::set_size( n + 1 );
            }
            return n;
        }
        else
        {
            size_t n = maxlen;
            auto nsize = size();
            if ( !reserve( nsize + n ) )
                return npos;
            auto pbegin = base_type::begin();
            for ( size_t i = 0; i < n; ++i, ++it )
            {
                new ( pbegin + nsize + i ) T( *it );
            }
            base_type::set_size( nsize + n );
            return nsize + n;
        }
    }

    // if it's a string
    template<class Iter, bool bNull = HasNullValue, typename = std::enable_if_t<bNull, void>>
    size_t push_back_iter( Iter it )
    {
        auto n = size();
        for ( ; *it != NullValue; ++n )
        {
            if ( !reserve( n + 1 ) )
                break;
            new ( begin() + n ) T( *it );
            base_type::set_size( n + 1 );
        }
        return n;
    }

    template<class Iter>
    size_t push_back_moveiter( Iter it, Iter itEnd )
    {
        size_t n = std::distance( it, itEnd );
        auto nsize = size();
        if ( !reserve( nsize + n ) )
            return npos;
        auto pbegin = base_type::begin();
        for ( size_t i = 0; i < n; ++i, ++it )
        {
            new ( pbegin + nsize + i ) T( std::move( *it ) );
        }
        base_type::set_size( nsize + n );
        return nsize + n;
    }

    void erase( iterator it, iterator itEnd )
    {
        auto n = std::distance( it, itEnd );
        if ( n > 0 )
        {
            for ( auto pend = end(); itEnd != pend; ++itEnd, ++it ) // todo fix bug
                std::swap( *it, *itEnd );
            for ( ; it != itEnd; ++it )
                it->~T();
            base_type::set_size( size() - n );
        }
    }

    template<class Iter>
    int compare( Iter it, size_t n ) const
    {
        auto it0 = begin(), it1 = end();
        for ( size_t i = 0; i < n; ++it, ++i, ++it0 )
        {
            if constexpr ( is_string )
            {
                if ( *it == NullValue )
                    break;
            }
            if ( it0 == it1 )
                return -1;
            if ( *it == *it0 )
                continue;
            if ( *it0 < *it )
                return -1;
            return 1;
        }
        return it0 == it1 ? 0 : 1;
    }

    // todo: requires T() < *Iterable::begin(), T() == *Iterable::begin().
    template<class Iterable>
    int compare( const Iterable &a ) const
    {
        return compare( std::begin( a ), std::end( a ) );
    }

    template<class Iter>
    int compare( Iter i, Iter iEnd ) const
    {
        auto it = begin(), itEnd = end();
        for ( ; i != iEnd; ++i, ++it )
        {
            if ( it == itEnd )
                return -1;
            auto &v = *it;
            if ( *i == v )
                continue;
            if ( v < *i )
                return -1;
            return 1;
        }
        return it == itEnd ? 0 : 1;
    }

    // todo: requires T() < *Iterable::begin(), T() == *Iterable::begin().
    template<class Iterable>
    bool operator==( const Iterable &a ) const
    {
        return compare( a ) == 0;
    }

    // todo: requires T() < *Iterable::begin(), T() == *Iterable::begin().
    template<class Iterable>
    bool operator!=( const Iterable &a ) const
    {
        return compare( a ) != 0;
    }

    // todo: requires T() < *Iterable::begin(), T() == *Iterable::begin().
    template<class Iterable>
    bool operator<=( const Iterable &a ) const
    {
        return compare( a ) <= 0;
    }

    // todo: requires T() < *Iterable::begin(), T() == *Iterable::begin().
    template<class Iterable>
    bool operator<( const Iterable &a ) const
    {
        return compare( a ) < 0;
    }

    template<class U, size_t M>
    bool operator==( U ( &a )[M] ) const
    {
        return 0 == compare( &a[0], M );
    }
};


template<class T, size_t N = 0, class Alloc = std::allocator<T>>
using Vector = VectorBase<T, N, Alloc, true, false>;

template<class T, size_t N>
using Array = VectorBase<T, N, void, true, false>;

template<class T, size_t N, class Alloc = std::allocator<T>>
using CStr = VectorBase<T, N, Alloc, true, true>;

template<size_t N>
using InplaceCStr = VectorBase<char, N, void, true, true>;

template<size_t N = 32, class Alloc = std::allocator<char>>
using CharCStr = VectorBase<char, N, Alloc, true, true>;

using String = CharCStr<>;

template<size_t N = 32, class Alloc = std::allocator<char>>
using WCharCStr = VectorBase<wchar_t, N, Alloc, true, true>;
using WString = WCharCStr<>;
} // namespace ftl
