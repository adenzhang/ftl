#pragma once
#include <type_traits>
#include <memory>
#include <cstring>

namespace ftl
{

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

private:
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

    this_type sub( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        if ( pos >= base_type::m_size )
            return {};
        maxlen = std::min( maxlen, base_type::m_size - pos );
        return this_type( base_type::m_data + pos, maxlen, m_cap - pos );
    }


private:
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
arrayview<const char, false> make_view( const char *p )
{
    return {p, std::strlen( p )};
}

/// T: element type.
/// N: inplace element capacity.
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
        return N;
    }
    constexpr size_t max_size() const
    {
        return HasNullValue ? ( N - 1 ) : N; // todo if HasNull, capacity-1;
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
        return n <= max_size();
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
        return N;
    }
    constexpr size_t max_size() const
    {
        return HasNullValue ? ( N - 1 ) : N; // todo if HasNull, capacity-1;
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
        return n <= max_size();
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
            m_alloc.deallocate( m_begin, m_capacity );
    }

    T *begin()
    {
        return m_begin;
    }
    size_t capacity() const
    {
        return m_capacity;
    }
    size_t max_size() const
    {
        return HasNullValue ? ( capacity() - 1 ) : capacity();
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
        if ( n <= max_size() )
            return true;
        auto newCap = HasNullValue ? ( n + 1 ) : n;
        if ( m_size == max_size() )
            newCap = std::max( 2 * m_capacity, newCap ); // double capacity
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
            m_alloc.deallocate( m_begin, m_capacity );
        m_begin = pBuf;
        m_capacity = newCap;
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
        arrayview<T> res{m_begin, m_size, m_capacity};
        m_begin = reinterpret_cast<T *>( &m_data[0] );
        m_size = 0;
        m_capacity = N;
        return res;
    }

    // user must check capacity before calling this function
    void movein( arrayview<T> &s )
    {
        static_cast<Impl *>( this )->~Impl();
        m_begin = s.begin();
        m_size = s.size();
        m_capacity = s.capacity();
        if constexpr ( HasNullValue )
            m_begin[m_size] = NullValue;
    }

private:
    Alloc m_alloc;
    T *m_begin = nullptr;
    size_type m_size = 0;
    size_type m_capacity = N;
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
            m_alloc.deallocate( m_begin, m_capacity );
    }

    T *begin()
    {
        return reinterpret_cast<T *>( m_begin );
    }
    size_type capacity() const
    {
        return m_capacity;
    }
    size_type size() const
    {
        return m_size;
    }
    size_type max_size() const
    {
        return HasNullValue ? ( capacity() - 1 ) : capacity();
    }

    bool reserve( size_t n )
    {
        if ( n <= max_size() )
            return true;
        auto newCap = HasNullValue ? ( n + 1 ) : n;
        if ( m_size == max_size() )
            newCap = std::max( 2 * m_capacity, newCap ); // double capacity
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
            m_alloc.deallocate( m_begin, m_capacity );
        m_begin = pBuf;
        m_capacity = newCap;
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
        arrayview<T> res{m_begin, m_size, m_capacity};
        m_begin = nullptr;
        m_size = 0;
        m_capacity = 0;
        return res;
    }

    void movein( arrayview<T> &s )
    // user must check capacity before calling this function
    {
        static_cast<Impl *>( this )->~Impl();
        m_begin = s.begin();
        m_size = s.size();
        m_capacity = s.capacity();
        if constexpr ( HasNullValue )
            m_begin[m_size] = NullValue;
    }

private:
    Alloc m_alloc;
    T *m_begin = nullptr;
    size_type m_size = 0;
    size_type m_capacity = 0;
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

    using base_type::begin;
    using base_type::capacity;
    using base_type::max_size;
    using base_type::reserve;
    using base_type::size;

    using iterator = pointer_type;
    using const_iterator = const T *;

    static constexpr size_t inplace_capacity = N;
    static constexpr bool is_string = HasNullValue;
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

    // it's a string todo
    //    template<class Iter, typename std::enable_if_t<HasNullValue && std::is_same_v<const decltype( *std::declval<Iter>() ), const T &>>>
    //    VectorBase( Iter it )
    //    {
    //        push_back_iter( it );
    //    }

    VectorBase( size_t n, const T &v )
    {
        push_back( v, n );
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
        if ( !a.using_inplace() && ( !is_string || a.capacity() > a.size() ) )
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
    //             typename = std::enable_if_t<!std::is_same_v<A, Alloc> || std::is_same_v<void, A>, void>>
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

    const arrayview<const T> sub_view( size_t pos, size_t maxlen = std::numeric_limits<size_t>::max() ) const
    {
        auto nsize = size();
        if ( pos >= nsize )
            return {};
        maxlen = std::min( maxlen, nsize - pos );
        return {begin() + pos, maxlen, capacity()};
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
        set_size( nsize - n );
        return nsize - n;
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

    // if it's a string
    template<class Iter, bool bNull = HasNullValue, typename = std::enable_if_t<bNull, void>>
    size_t push_back_iter( Iter it )
    {
        auto n = size();
        for ( auto pbegin = begin(); *it != NullValue; ++n )
        {
            if ( !reserve( n + 1 ) )
                break;
            new ( pbegin + n ) T( *it );
        }
        base_type::set_size( n );
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

    template<class Pred>
    iterator remove_if( Pred &&pred )
    {
        auto newEnd = begin(), itEnd = end();
        for ( auto it = newEnd; it != itEnd; ++it )
        {
            if ( pred( *it ) )
            {
                if ( newEnd == it ) // first one to remove
                    ++newEnd;
            }
            else
            {
                if ( newEnd != it )
                    std::swap( *newEnd, *it );
                ++newEnd;
            }
        }
        return newEnd;
    }

    void erase( iterator it, iterator itEnd )
    {
        auto n = std::distance( it, itEnd );
        if ( n > 0 )
        {
            for ( ; it != itEnd; ++it )
                it->~T();
            set_size( size() - n );
        }
    }

    template<class Iterable>
    int compare( const Iterable &a ) const
    {
        auto it = begin(), itEnd = end();
        for ( const auto &e : a )
        {
            if ( it == itEnd )
                return -1;
            auto &v = *( it++ );
            if ( e == v )
                continue;
            if ( v < e )
                return -1;
            else
                return 1;
        }
        return it == itEnd ? 0 : 1;
    }

    template<class Iter>
    int compare( Iter i, Iter iEnd ) const
    {
        auto it = begin(), itEnd = end();
        for ( ; i != iEnd; ++i )
        {
            if ( it == itEnd )
                return -1;
            auto &v = *( it++ );
            if ( *i == v )
                continue;
            if ( v < *i )
                return -1;
            else
                return 1;
        }
        return it == itEnd ? 0 : 1;
    }

    template<class Iterable>
    bool operator==( const Iterable &a ) const
    {
        return compare( a ) == 0;
    }

    template<class Iterable>
    bool operator!=( const Iterable &a ) const
    {
        return compare( a ) != 0;
    }

    template<class Iterable>
    bool operator<=( const Iterable &a ) const
    {
        return compare( a ) <= 0;
    }

    template<class Iterable>
    bool operator<( const Iterable &a ) const
    {
        return compare( a ) < 0;
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
