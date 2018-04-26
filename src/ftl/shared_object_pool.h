#ifndef _SHAREDOBJECTPOOL_H_
#define _SHAREDOBJECTPOOL_H_

#include <memory>
#include <deque>
#include <cassert>
#include <iostream>

////============================

// write an expression in terms of type 'T' to create a trait which is true for a type which is valid within the expression
#define FTL_CHECK_EXPR( traitsName, ... )                                                                                                           \
    template<typename U = void>                                                                                                                     \
    struct traitsName                                                                                                                               \
    {                                                                                                                                               \
        using type = traitsName;                                                                                                                    \
                                                                                                                                                    \
        template<class T>                                                                                                                           \
        static auto Check( int ) -> decltype( __VA_ARGS__, bool() );                                                                                \
                                                                                                                                                    \
        template<class>                                                                                                                             \
        static int Check( ... );                                                                                                                    \
                                                                                                                                                    \
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;                                                           \
    }

#define FTL_HAS_MEMBER( member, name ) FTL_CHECK_EXPR( name, ::std::declval<T>().member )

#define FTL_IS_COMPATIBLE_FUNC_ARG( func, name ) FTL_CHECK_EXPR( name, func(::std::declval<T>() ) )

#define FTL_IS_COMPATIBLE_FUNC_ARG_LVALUE( func, name ) FTL_CHECK_EXPR( name, func(::std::declval<T &>() ) )

#define FTL_CHECK_EXPR_TYPE( traitsName, ... )                                                                                                      \
    template<typename R, typename U = void>                                                                                                         \
    struct traitsName                                                                                                                               \
    {                                                                                                                                               \
        using type = traitsName;                                                                                                                    \
                                                                                                                                                    \
        template<class T>                                                                                                                           \
        static std::enable_if_t<std::is_same<R, decltype( __VA_ARGS__ )>::value, bool> Check( int );                                                \
                                                                                                                                                    \
        template<class>                                                                                                                             \
        static int Check( ... );                                                                                                                    \
                                                                                                                                                    \
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;                                                           \
    }

#define FTL_HAS_MEMBER_TYPE( member, name ) FTL_CHECK_EXPR_TYPE( name, ::std::declval<T>().member )

////============================

namespace ftl
{

//! forward gen_iterator
//! requires ContainerT define types:
//!     class node_type which has operator bool() and operator==()
//!     class value_type
//!
//! and implement methods:
//!     bool Container::next(node_type&)
//!     bool Container::is_end(const node_type&)
//!     value_type* Container::get_value(const node_type&) const
//!
template<typename ContainerT,
         bool isConstIter = false,
         typename ValueT = std::conditional_t<isConstIter, const typename ContainerT::value_type, typename ContainerT::value_type>,
         typename NodeT = typename ContainerT::node_type>
struct gen_iterator
{
    using this_type = gen_iterator;
    using iter_type = this_type;
    using container_type = ContainerT;

    using value_type = std::remove_const_t<ValueT>;
    using node_type = NodeT;
    using iter_value_type = value_type;

    node_type mNode;
    container_type *mContainer;

    // end iterator is constructed with defaults
    gen_iterator( node_type node = node_type(), container_type *con = nullptr )
        : mNode( node )
        , mContainer( con )
    {
        assert( con || !node );
    }

    bool operator==( const this_type &a ) const
    {
        if ( mNode == a.mNode )
            return true;
        if ( !mNode )
            return is_end( *a.mContainer, a.mNode );
        if ( !a.mNode )
            return is_end( *mContainer, mNode );
        return false;
    }
    iter_type &operator++()
    {
        next( *mContainer, mNode );
        return *this;
    }
    iter_value_type *operator->()
    {
        return get_value( *mContainer, mNode );
    }

    //-- derived operators
    bool operator!=( const this_type &a ) const
    {
        return !operator==( a );
    }

    iter_type operator++( int )
    {
        iter_type r = *static_cast<iter_type *>( this );
        operator++();
        return r;
    }
    iter_value_type *operator->() const
    {
        return const_cast<this_type &>( *this ).operator->();
    }
    iter_value_type &operator*()
    {
        return *operator->();
    }
    iter_value_type &operator*() const
    {
        return const_cast<this_type &>( *this ).operator*();
    }

    //-- others
    node_type &get_node()
    {
        return mNode;
    }

protected:
    FTL_CHECK_EXPR_TYPE( has_member_next, std::declval<T>().next( std::declval<node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_next, next( std::declval<T>(), std::declval<node_type>() ) );

    template<typename C, typename N>
    std::enable_if_t<has_member_next<bool, C>::value, bool> next( N &n )
    {
        return mContainer->next( n );
    }
    template<typename C, typename N>
    std::enable_if_t<has_global_next<bool>::value, bool> next( N &n )
    {
        return next( *mContainer, n );
    }


    FTL_CHECK_EXPR_TYPE( has_member_is_end, std::declval<T>().is_end( std::declval<const node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_is_end, is_end( std::declval<T>(), std::declval<const node_type>() ) );

    template<typename C, typename N>
    std::enable_if_t<has_member_is_end<bool, C>::value, bool> is_end( const N &n )
    {
        return mContainer->is_end( n );
    }
    template<typename C, typename N>
    std::enable_if_t<has_global_is_end<bool>::value, bool> is_end( const N &n )
    {
        return is_end( *mContainer, n );
    }

    FTL_CHECK_EXPR_TYPE( has_member_get_value, std::declval<T>().get_value( std::declval<const node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_get_value, get_value( std::declval<T>(), std::declval<const node_type>() ) );

    template<typename C, typename N>
    std::enable_if_t<has_member_get_value<value_type *, C>::value, bool> get_value( const N &n )
    {
        return mContainer->get_value( n );
    }
    template<typename C, typename N>
    std::enable_if_t<has_global_get_value<value_type *>::value, bool> get_value( const N &n )
    {
        return get_value( *mContainer, n );
    }
};

//! intrusive_singly_list
//! type T must have
//! - node_type& get_node()
//!

struct simple_singly_list
{
    using this_type = simple_singly_list;
    struct data_type
    {
        data_type *next = nullptr;

        void reset()
        {
            next = nullptr;
        }
    };
    using value_type = data_type *;
    using node_type = data_type *;

    node_type mHead = nullptr;

    this_type &push_front( value_type &n )
    {
        if ( n )
        {
            n->next = mHead;
            mHead = n;
        }
        return *this;
    }

    template<typename F>
    node_type find_if( F &&func ) const
    {
        for ( auto p = mHead; p; p = p->next )
            if ( func( p ) )
                return p;
        return nullptr;
    }

    node_type pop_front()
    {
        node_type r = mHead;
        if ( r )
            mHead = r->next;
        return r;
    }
    this_type &push( value_type &n )
    {
        return push_front( n );
    }
    node_type pop()
    {
        return pop_front();
    }

    node_type insert_after( node_type pos, value_type d )
    {
        d->next = pos->next;
        pos->next = d;
        return d;
    }
    void remove_after( node_type node )
    {
        if ( node->next )
        {
            auto p = node->next;
            node->next = node->next->next;
            p->reset();
        }
    }

    void remove( node_type node )
    {
        if ( !node )
            return;
        if ( node == mHead )
        {
            mHead = node->next;
            node->reset();
            return;
        }
        else
        {
            node_type prev = find_if( [&]( node_type &n ) { return n->next == node; } );
            remove_after( prev );
        }
    }

    //- iterator support

    using iterator = gen_iterator<this_type, false>;
    using const_iterator = gen_iterator<this_type, true>;

    bool is_end( const node_type &n ) const
    {
        return !n;
    }
    bool next( node_type &n )
    {
        if ( !n )
            return false;
        n = n->next;
        return true;
    }
    value_type *get_value( const node_type &n ) const
    {
        return &const_cast<node_type &>( n );
    }
    iterator begin()
    {
        return {mHead, this};
    }
    iterator end()
    {
        return {node_type(), this};
    }

    //- modify by iterator

    void erase( iterator it )
    {
        remove( it.get_node() );
    }
    iterator insert_after( iterator it, node_type n )
    {
        return {insert_after( it.get_node(), n ), this};
    }
};


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
    bool next( node_type &n )
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

    const_iterator cbegin()
    {
        if ( empty() )
            return const_iterator();
        return const_iterator( pFront, this );
    }
    const_iterator cend()
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

    bool empty()
    {
        return mSize == 0;
    }

    bool full()
    {
        return mSize == CapacityT;
    }
    void clear()
    {
        while ( !empty() )
            pop_front();
    }
    size_t size()
    {
        return mSize;
    }
};

template<typename T>
class safe_enable_shared_from_this : public std::enable_shared_from_this<T>
{
public:
    template<typename... _Args>
    static ::std::shared_ptr<T> create_me( _Args &&... p_args )
    {
        return ::std::allocate_shared<T>( Alloc(), std::forward<_Args>( p_args )... );
    }

protected:
    struct Alloc : std::allocator<T>
    {
        template<typename _Up, typename... _Args>
        void construct( _Up *__p, _Args &&... __args )
        {
            ::new ( static_cast<void *>( __p ) ) _Up( std::forward<_Args>( __args )... );
        }
    };
    safe_enable_shared_from_this() = default;
    safe_enable_shared_from_this( const safe_enable_shared_from_this & ) = delete;
    safe_enable_shared_from_this &operator=( const safe_enable_shared_from_this & ) = delete;
};

template<typename ChildT, typename Object, typename ObjectAlloc = std::allocator<Object>, typename QueueT = std::deque<Object *>>
class shared_object_pool_base
{
public:
    typedef shared_object_pool_base this_type;
    // should be protected
    shared_object_pool_base( const ObjectAlloc &alloc = ObjectAlloc() )
        : objectAlloc( alloc )
        , allocatedCount( 0 )
    {
    }

    static const size_t obj_size = sizeof( Object );

    friend ChildT;

protected:
    typedef QueueT ObjectQ;
    ObjectAlloc objectAlloc;
    ObjectQ freeQ;
    size_t allocatedCount = 0;
    size_t freeCount = 0; // for debug only
public:
    struct object_destroy;
    friend object_destroy;
    typedef std::shared_ptr<Object> shared_object_ptr;
    typedef std::unique_ptr<Object, object_destroy> unique_object_ptr;
    typedef std::shared_ptr<ChildT> Ptr;

    struct default_destroy
    {
        void operator()( Object *p )
        {
            if ( p )
                p->~Object();
        }
    };

    struct object_destroy
    {
        Ptr pPool; // ensure pool existence
        object_destroy( const Ptr &p = nullptr )
            : pPool( p )
        {
        }
        object_destroy( const object_destroy &d )
            : pPool( d.pPool )
        {
            //            std::cout << "return_object(return_object&)" << std::endl;
        }
        object_destroy( object_destroy &&d )
            : pPool( std::move( d.pPool ) )
        {
        }
        object_destroy &operator=( const object_destroy &d )
        {
            pPool = d.pPool;
            return *this;
        }
        object_destroy &operator=( object_destroy &&d )
        {
            pPool = std::move( d.pPool );
            return *this;
        }

        void operator()( Object *p )
        {
            assert( p && pPool || !p );
            // return it to freeQ
            if ( p )
            { // todo lock
                ++pPool->freeCount;
                default_destroy destroy;
                destroy( p );
                pPool->freeQ.push_back( p );
            }
            //            std::cout << "destroy Object: allocated:" << pPool->allocatedCount << ", free size:" << pPool->freeQ.size() << std::endl;
        }
    };

    template<bool bOneByOne = true>
    void destroy()
    {
        assert( allocatedCount == freeCount );
        //        std::cout << "~shared_object_pool_base, allocated size:" << allocatedCount << " == freeQ size:" << freeQ.size() << std::endl;
        if ( freeQ.size() )
        { // todo lock
            for ( typename ObjectQ::iterator it = freeQ.begin(); it != freeQ.end(); ++it )
            {
                objectAlloc.deallocate( *it, 1 );
            }
            freeQ.clear();
        }
    }

    template<typename... Args>
    shared_object_ptr create_shared( Args &&... args )
    {
        if ( auto p = static_cast<ChildT *>( this )->get_or_allocate() )
        {
            new ( p ) Object( std::forward<Args>( args )... );
            return {p, object_destroy( static_cast<ChildT *>( this )->me_ptr() )};
        }
        else
            return {};
    }

    template<typename... Args>
    unique_object_ptr create_unique( Args &&... args )
    {
        if ( auto p = static_cast<ChildT *>( this )->get_or_allocate() )
        {
            new ( p ) Object( std::forward<Args>( args )... );
            return {p, object_destroy( static_cast<ChildT *>( this )->me_ptr() )};
        }
        else
            return {nullptr, {}};
    }

    size_t size() const
    {
        return freeQ.size();
    }

    size_t allocate( size_t n = 1 )
    {
        for ( size_t i = 0; i < n; ++i )
        {
            if ( auto p = objectAlloc.allocate( 1 ) )
                freeQ.push_back( p );
            else
                return i;
        }
        return n;
    }

    void purge( size_t nReserve = 0 )
    {
        { // todo lock
            while ( nReserve < freeQ.size() )
            {
                objectAlloc.deallocate( freeQ.front, 1 );
                freeQ.pop_front();
            }
        }
    }
};


template<typename ChildT,
         typename Object,
         size_t CapacityT,
         typename ObjectAlloc = std::allocator<Object>,
         typename QueueT = circular_queue<Object *, CapacityT>>
class fixed_shared_pool_base : public shared_object_pool_base<ChildT, Object, ObjectAlloc, QueueT>
{
protected:
    Object *mObjects;

public:
    using this_type = fixed_shared_pool_base;
    using base_type = shared_object_pool_base<ChildT, Object, ObjectAlloc, QueueT>;

    static const size_t capacity = CapacityT;

    // should be protected
    fixed_shared_pool_base( const ObjectAlloc &alloc = ObjectAlloc() )
        : base_type( alloc )
    {
        mObjects = base_type::objectAlloc.allocate( CapacityT, nullptr );
        for ( size_t i = 0; i < CapacityT; ++i )
            base_type::freeQ.push_back( mObjects + i );
    }

    ~fixed_shared_pool_base()
    {
        assert( base_type::freeQ.size() == CapacityT );
        std::cout << "~fixed_shared_pool_base, allocated size:" << base_type::allocatedCount << " == freeQ size:" << base_type::freeQ.size()
                  << std::endl;
        if ( mObjects )
        { // todo lock
            base_type::objectAlloc.deallocate( mObjects, CapacityT );
            mObjects = nullptr;
        }
    }


    size_t size() const
    {
        return base_type::freeQ.size();
    }

    size_t allocate( size_t n )
    {
        return 0;
    }

    void purge( size_t = 0 )
    {
    }
};

template<typename Object, typename ObjectAlloc = std::allocator<Object>>
class shared_object_pool : public shared_object_pool_base<shared_object_pool<Object, ObjectAlloc>, Object, ObjectAlloc>,
                           public safe_enable_shared_from_this<shared_object_pool<Object, ObjectAlloc>>
{
public:
    using this_type = shared_object_pool;
    using base_type = shared_object_pool_base<this_type, Object, ObjectAlloc>;

    friend struct safe_enable_shared_from_this<this_type>::Alloc;

    shared_object_pool( const ObjectAlloc &alloc = ObjectAlloc() )
        : base_type( alloc )
    {
    }
    ~shared_object_pool()
    {
        static_cast<base_type *>( this )->destroy();
    }

    auto me_ptr()
    {
        return this->shared_from_this();
    }

    Object *get_or_allocate()
    {
        // todo lock
        Object *p = nullptr;
        if ( !base_type::freeQ.empty() )
        {
            p = base_type::freeQ.front();
            base_type::freeQ.pop_front();
        }
        else
        {
            // if freeQ empty
            ++base_type::allocatedCount;
            p = base_type::objectAlloc.allocate( 1, nullptr );
        }
        return p;
    }
};

template<typename Object, size_t CapacityT, typename ObjectAlloc = std::allocator<Object>>
class fixed_shared_pool : public fixed_shared_pool_base<fixed_shared_pool<Object, CapacityT, ObjectAlloc>, Object, CapacityT, ObjectAlloc>,
                          public safe_enable_shared_from_this<fixed_shared_pool<Object, CapacityT, ObjectAlloc>>
{
public:
    using this_type = fixed_shared_pool;
    using base_type = fixed_shared_pool_base<this_type, Object, CapacityT, ObjectAlloc>;

    friend struct safe_enable_shared_from_this<this_type>::Alloc;

    fixed_shared_pool( const ObjectAlloc &alloc = ObjectAlloc() )
        : base_type( alloc )
    {
    }

    auto me_ptr()
    {
        return this->shared_from_this();
    }

    Object *get_or_allocate()
    {
        // todo lock
        Object *p = nullptr;
        if ( !base_type::freeQ.empty() )
        {
            p = base_type::freeQ.front();
            base_type::freeQ.pop_front();
        }
        return p;
    }
};
}
#endif // _SHAREDOBJECTPOOL_H_
