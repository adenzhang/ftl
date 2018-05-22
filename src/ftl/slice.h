#pragma once
#include <memory>
#include <cassert>

namespace ftl
{

template<typename T, typename AllocT = std::allocator<T>, typename PtrT = std::shared_ptr<T>>
struct slice_node
{
    using size_type = size_t;
    using value_type = T;
    using pointer = T *;

    using allocator_type = AllocT;

    using SPtr = PtrT;

    SPtr mPtr;
    size_type mCap = 0;
    allocator_type mAlloc;

    using this_type = slice_node<value_type, allocator_type, SPtr>;

    template<class Deleter = std::default_delete<T>>
    slice_node( T *p = nullptr, size_type capacity = 0, const Deleter &d = Deleter(), const AllocT &alloc = AllocT() )
        : mPtr( p, d )
        , mCap( capacity )
        , mAlloc( alloc )
    {
    }

    template<size_t N, class Deleter = std::default_delete<T>>
    slice_node( T p[N], Deleter d = Deleter(), const AllocT &alloc = AllocT() )
        : mPtr( p, d )
        , mCap( N )
        , mAlloc( alloc )
    {
    }

    slice_node( const std::shared_ptr<T> &p, size_type capacity, const AllocT &alloc = AllocT() )
        : mPtr( p )
        , mCap( capacity )
        , mAlloc( alloc )
    {
    }

    slice_node( std::shared_ptr<T> &&p, size_type capacity, const AllocT &alloc = AllocT() )
        : mPtr( std::move( p ) )
        , mCap( capacity )
        , mAlloc( alloc )
    {
    }

    template<class Deleter>
    slice_node( std::unique_ptr<T, Deleter> &&p, size_type capacity, const AllocT &alloc = AllocT() )
        : mPtr( std::move( p ) )
        , mCap( capacity )
        , mAlloc( alloc )
    {
    }

    slice_node( const this_type &a )
        : mPtr( a.mPtr )
        , mCap( a.mCap )
        , mAlloc( a.mAlloc )
    {
    }

    slice_node( this_type &&a )
        : mPtr( std::move( a.mPtr ) )
        , mCap( a.mCap )
        , mAlloc( std::move( a.mAlloc ) )
    {
    }

    void reset()
    {
        mPtr.reset();
        mCap = 0;
    }

    this_type &operator=( const this_type &a )
    {
        mPtr = a.mPtr;
        mCap = a.mCap;
        mAlloc = a.mAlloc;
        return *this;
    }

    this_type &operator=( this_type &&a )
    {
        mPtr = std::move( a.mPtr );
        mCap = a.mCap;
        mAlloc = std::move( a.mAlloc );
        return *this;
    }

    bool operator==( const this_type &a ) const
    {
        return mPtr == a.mPtr;
    }

    bool operator!=( const this_type &a ) const
    {
        return !operator==( a );
    }

    //! reallocate returns old pointer
    SPtr reallocate( size_t n, size_t nCopy, size_t nCopyPos = 0 )
    {
        assert( nCopyPos + nCopy <= n );
        assert( nCopyPos + nCopy <= mCap );
        assert( nCopyPos <= mCap );

        pointer pp = mAlloc.allocate( n, nullptr );
        SPtr ptr( pp, [this, n]( pointer p ) { mAlloc.deallocate( p, n ); }, mAlloc );
        mPtr.swap( ptr );
        mCap = n;

        pointer p0 = &( *ptr );
        pointer p = begin() + nCopyPos;
        for ( size_type i = 0; i < nCopy; ++i, ++p, ++p0 )
            new ( p ) value_type( *p0 );

        return ptr;
    }

    constexpr value_type *begin()
    {
        return &( *mPtr );
    }
    constexpr const value_type *cbegin() const
    {
        return &( *mPtr );
    }

    this_type copy( size_t pos, size_t len, size_t cap ) const
    {
        this_type a( *this );
        a.reallocate( cap, len, pos );
        return a;
    }
};

template<typename T, typename AllocT = std::allocator<T>, typename PtrT = std::shared_ptr<T>>
class slice
{
public:
    using size_type = size_t;
    using value_type = T;
    using pointer = T *;

    using iterator = pointer;
    using const_iterator = const T *;
    using allocator_type = AllocT;

    using SPtr = PtrT;

    using node_type = slice_node<T, AllocT, PtrT>;
    using node_ptr = std::shared_ptr<node_type>;
    using this_type = slice<value_type, allocator_type, SPtr>;

    template<class Deleter = std::default_delete<T>>
    slice( T *p = nullptr,
           size_t offset = 0,
           size_type size = 0,
           size_type capacity = 0,
           const Deleter &d = Deleter(),
           const AllocT &alloc = AllocT() )
        : mNode( std::make_shared<node_type>( p, capacity == 0 ? size : capacity, d, alloc ) )
        , mOffset( offset )
        , mSize( size )
    {
    }

    template<size_t N, class Deleter = std::default_delete<T>>
    slice( T p[N], size_t offset, size_type size, Deleter d = Deleter(), const AllocT &alloc = AllocT() )
        : mNode( std::make_shared<node_type>( p, N, d, alloc ) )
        , mOffset( offset )
        , mSize( size )
    {
    }

    slice( const std::shared_ptr<T> &p, size_t offset, size_type size, size_type capacity = 0, const AllocT &alloc = AllocT() )
        : mNode( std::make_shared<node_type>( p, capacity == 0 ? size : capacity, alloc ) )
        , mOffset( offset )
        , mSize( size )
    {
    }

    slice( std::shared_ptr<T> &&p, size_t offset, size_type size, size_type capacity = 0, const AllocT &alloc = AllocT() )
        : mNode( std::make_shared<node_type>( std::move( p ), capacity == 0 ? size : capacity, alloc ) )
        , mOffset( offset )
        , mSize( size )
    {
    }

    template<class Deleter>
    slice( std::unique_ptr<T, Deleter> &&p, size_t offset, size_type size, size_type capacity = 0, const AllocT &alloc = AllocT() )
        : mNode( std::make_shared<node_type>( std::move( p ), capacity == 0 ? size : capacity, alloc ) )
        , mOffset( offset )
        , mSize( size )
    {
    }

    slice( const this_type &a )
        : mNode( a.mNode )
        , mOffset( a.mOffset )
        , mSize( a.mSize )
    {
    }

    slice( this_type &&a )
        : mNode( std::move( a.mNode ) )
        , mOffset( a.mOffset )
        , mSize( a.mSize )
    {
    }

    void reset()
    {
        mNode.reset();
        mSize = 0;
    }

    this_type &operator=( const this_type &a )
    {
        mNode = a.mNode;
        mOffset = a.mOffset;
        mSize = a.mSize;
        return *this;
    }

    this_type &operator=( this_type &&a )
    {
        mNode = std::move( a.mNode );
        mOffset = a.mOffset;
        mSize = a.mSize;
        return *this;
    }

    bool operator==( const this_type &a ) const
    {
        return mNode == a.mNode && mSize == a.mSize && mOffset == a.mOffset;
    }

    bool operator!=( const this_type &a ) const
    {
        return !operator==( a );
    }


    value_type &operator[]( size_t k )
    {
        return *( begin() + k );
    }

    const value_type &operator[]( size_t k ) const
    {
        return *( cbegin() + k );
    }


    constexpr size_type capacity() const
    {
        return mNode->mCap;
    }

    constexpr size_type size() const
    {
        return mSize;
    }

    bool reserve( size_type n )
    {
        if ( mOffset + n > capacity() )
        {
            return set_capacity( n, mSize + mOffset );
        }
        return true;
    }

    //! resize will destruct/construct elements. And will potentially auto increase capacity
    //! if value_type is not default constructible, expanding resize will fail.
    template<typename U = value_type>
    std::enable_if_t<std::is_trivial_v<U>, bool> resize( size_type n, bool bAutoExpand = false )
    {
        if ( n < mSize )
        {
            mSize = n;
            return true;
        }
        else if ( n > mSize )
        {
            return bAutoExpand && expand( n - mSize );
        }
        return true;
    }

    template<typename U = value_type>
    std::enable_if_t<!std::is_trivial_v<U>, bool> resize( size_type n, bool bAutoExpand = false )
    {
        if ( n < mSize )
        {
            auto it = begin() + n;
            for ( size_t i = 0, N = mSize - n; i < N; ++i, ++it )
                destruct( it );
            mSize = n;
            return true;
        }
        else if ( n > mSize )
        {
            return bAutoExpand && expand( n - mSize );
        }
        return true;
    }

    //! reshape set size only, will not destruct/construct.
    bool reshape( int offset, size_type size )
    {
        if ( offset + mOffset < 0 )
            return false;
        if ( offset + mOffset + size > capacity() )
            return false;
        mOffset += offset;
        mSize = size;
        return true;
    }

    size_type get_offset() const
    {
        return mOffset;
    }

    void swap( slice &a )
    {
        mNode.swap( a.mNode );
        std::swap( mSize, a.mSize );
    }

    // unsafe, emplace_back without boundary check.
    template<class... Args>
    value_type &emplace_back( Args &&... args )
    {
        assert( mSize < capacity() );
        pointer p = end();
        new ( p ) value_type( std::forward<Args...>( args )... );
        ++mSize;
        return *p;
    }

    void push_back( const value_type &a, size_t n = 1 )
    {
        for ( size_t i = 0; i < n; ++i )
            push_back( a );
    }

    //! expand size & capacity, reallocate if possible
    //! construct new elements if constructible
    template<typename... Args>
    std::enable_if_t<std::is_constructible_v<value_type, Args...>, bool> expand( size_type n, Args... args )
    {
        if ( n + mSize + mOffset > capacity() )
        {
            set_capacity( n + mSize + mOffset, mSize + mOffset );
        }
        for ( size_t i = 0; i < n; ++i )
            emplace_back( std::forward<Args...>( args )... );
        return true;
    }

    // expand capacity, reallocate if possible
    // size will not grow as constrution is not applicable
    template<typename... Args>
    std::enable_if_t<!std::is_constructible_v<value_type, Args...>, bool> expand( size_type n, Args... )
    {
        if ( n + mSize + mOffset > capacity() )
        {
            set_capacity( n + mSize + mOffset, mSize + mOffset );
        }
        mSize += n;
        return true;
    }

    constexpr iterator begin()
    {
        return mNode->begin() + mOffset;
    }

    constexpr iterator end()
    {
        return begin() + mSize;
    }
    constexpr const_iterator cbegin()
    {
        return mNode->cbegin() + mOffset;
    }

    constexpr const_iterator cend()
    {
        return cbegin() + mSize;
    }

    void erase_at( size_t k )
    {
        destruct( begin() + k );
    }

    value_type *data()
    {
        return mNode->begin();
    }

    this_type copy( int fromPos, size_type size, size_type cap = 0 ) const
    {
        this_type a;
        a.mNode = std::move( std::make_shared<node_type>( mNode->copy( mOffset + fromPos, size, cap == 0 ? size : cap ) ) );
        a.mOffset = 0;
        a.mSize = size;
        return std::move( a );
    }

protected:
    //! set_capacity_size expands/shrinks capacity and shrinks size.
    //! All new created elements will be default constructed.
    bool set_capacity( size_type cap, size_type sizeToKeep )
    {
        assert( sizeToKeep <= cap );
        assert( sizeToKeep <= mSize + mOffset );

        if ( cap != capacity() )
        {
            //            mNode->reallocate( cap, std::min( std::min( mOffset + mSize, cap ), mOffset + size ) );
            mNode->reallocate( cap, sizeToKeep );
        }
        return true;
    }

    void destruct( pointer p )
    {
        p->~value_type();
    }

    node_ptr mNode;
    size_type mOffset = 0;
    size_type mSize = 0;
};
}
