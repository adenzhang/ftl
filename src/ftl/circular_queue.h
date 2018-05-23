#include <ftl/intrusive_list.h>

namespace ftl {


template<typename T, size_t CapacityT, typename ObjectAlloc = std::allocator<T>>
class circular_queue
{
protected:
    ObjectAlloc mAlloc;
    T *mBuf = nullptr;
    T *pFront = nullptr; // points to first available
    T *pEnd = nullptr; // points to free memory block
    size_t mSize = 0;

    circular_queue( const circular_queue & ) = delete;
    circular_queue &operator=( const circular_queue & ) = delete;

public:
    using this_type = circular_queue;
    static const size_t capacity = CapacityT;

    //- iterator support

    using value_type = T;
    using node_type = T *;

    using iterator = gen_iterator<this_type, false>;
    using const_iterator = gen_iterator<this_type, true>;

    bool is_end( const node_type &n ) const
    {
        return !n;
    }
    bool next( node_type &n ) const
    {
        if ( !n )
            return false;
        if ( ++n == mBuf + CapacityT )
            n = mBuf;
        if ( n == pEnd || n == pFront )
            n = node_type();
        return true;
    }
    value_type *get_value( const node_type &n ) const
    {
        return n;
    }

    //--
    iterator begin()
    {
        if ( empty() )
            return iterator();
        return iterator( pFront, this );
    }
    iterator end()
    {
        return iterator();
    }

    const_iterator cbegin() const
    {
        if ( empty() )
            return const_iterator();
        return const_iterator( pFront, this );
    }
    const_iterator cend() const
    {
        return const_iterator();
    }

public:
    circular_queue( const ObjectAlloc &alloc = ObjectAlloc(), bool bDeferAllocate = false )
        : mAlloc( alloc )
        , mBuf( bDeferAllocate ? nullptr : mAlloc.allocate( CapacityT, nullptr ) )
        , pFront( mBuf )
        , pEnd( mBuf )
    {
    }
    circular_queue( circular_queue &&a )
        : mAlloc( std::move( a.mAlloc ) )
    {
        mBuf = a.mBuf;
        pFront = a.pFront;
        pEnd = a.pEnd;
        mSize = a.mSize;

        a.mBuf = nullptr;
        a.pFront = nullptr;
        a.pEnd = nullptr;
        a.mSize = 0;
    }
    circular_queue &operator=( circular_queue &&a )
    {
        mAlloc = std::move( a.mAlloc );
        mBuf = a.mBuf;
        pFront = a.pFront;
        pEnd = a.pEnd;
        mSize = a.mSize;

        a.mBuf = nullptr;
        a.pFront = nullptr;
        a.pEnd = nullptr;
        a.mSize = 0;
    }

    T *allocate()
    {
        assert( !mBuf );
        mBuf = mAlloc.allocate( CapacityT, nullptr );
        pFront = mBuf;
        pEnd = mBuf;
        return reinterpret_cast<T *>( mBuf );
    }

    ~circular_queue()
    {
        clear();
        if ( mBuf )
            mAlloc.deallocate( mBuf, CapacityT );
        mBuf = nullptr;
        //        mAlloc.deallocate( reinterpret_cast<T *>( pMem ), CapacityT );
    }

    template<class... Args>
    bool push_back( Args &&... args )
    {
        if ( full() )
            return false;
        new ( static_cast<void *>( pEnd ) ) T( std::forward<Args>( args )... );

        ++mSize;
        ++pEnd;
        if ( pEnd == mBuf + CapacityT )
            pEnd = mBuf;
        return true;
    }

    template<typename Iterator>
    size_t insert( Iterator it, Iterator end )
    {
        size_t i = 0;
        for ( ; it != end && push_back( *it ); ++i, ++it )
            ;
        return i;
    }

    bool pop_front()
    {
        if ( empty() )
            return false;
        reinterpret_cast<T *>( pFront )->~T();

        --mSize;
        ++pFront;
        if ( pFront == mBuf + CapacityT )
            pFront = mBuf;
        return true;
    }

    T &front()
    {
        return *reinterpret_cast<T *>( pFront );
    }

    bool empty() const
    {
        return mSize == 0;
    }

    bool full() const
    {
        return mSize == CapacityT;
    }
    void clear()
    {
        while ( !empty() )
            pop_front();
    }
    size_t size() const
    {
        return mSize;
    }
};
}
