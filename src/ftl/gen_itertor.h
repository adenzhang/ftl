#pragma once
#include <cassert>
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
    using container_type = std::conditional_t<isConstIter, const ContainerT, ContainerT>;

    using value_type = std::remove_const_t<ValueT>;
    using node_type = NodeT;
    using iter_value_type = std::conditional_t<isConstIter, const value_type, value_type>;

    FTL_CHECK_EXPR_TYPE( has_member_next, std::declval<T>().next( std::declval<node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_next, next( std::declval<T>(), std::declval<node_type>() ) );

    FTL_CHECK_EXPR_TYPE( has_member_prev, std::declval<T>().prev( std::declval<node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_prev, prev( std::declval<T>(), std::declval<node_type>() ) );

    FTL_CHECK_EXPR_TYPE( has_member_is_end, std::declval<T>().is_end( std::declval<const node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_is_end, is_end( std::declval<T>(), std::declval<const node_type>() ) );

    FTL_CHECK_EXPR_TYPE( has_member_get_value, std::declval<T>().get_value( std::declval<const node_type>() ) );
    FTL_CHECK_EXPR_TYPE( has_global_get_value, get_value( std::declval<T>(), std::declval<const node_type>() ) );

    node_type mNode;
    container_type *mContainer;

    // end iterator is constructed with defaults
    gen_iterator( node_type node = node_type(), container_type *con = nullptr )
        : mNode( node )
        , mContainer( con )
    {
        assert( con || !node );
    }

    template<class T>
    gen_iterator( const T &a )
        : mNode( a.mNode )
        , mContainer( a.mContainer )
    {
    }

    template<class T>
    this_type &operator=( const T &a )
    {
        mNode = a.mNode;
        mContainer = a.mContainer;
    }

    bool operator==( const this_type &a ) const
    {
        if ( mNode == a.mNode )
            return true;
        if ( !mNode )
            return a.mContainer->is_end( a.mNode );
        if ( !a.mNode )
            return mContainer->is_end( mNode );
        return false;
    }
    iter_type &operator++()
    {
        mContainer->next( mNode );
        return *this;
    }
    iter_value_type *operator->()
    {
        return mContainer->get_value( mNode );
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

    template<typename C = container_type, typename N = node_type>
    std::enable_if_t<has_member_prev<bool, C>::value, iter_value_type> operator--()
    {
        mContainer->prev( mNode );
        return *this;
    }
    template<typename C = container_type, typename N = node_type>
    std::enable_if_t<has_member_prev<bool, C>::value, iter_value_type> operator--( int )
    {
        iter_type r = *static_cast<iter_type *>( this );
        operator--();
        return r;
    }

    node_type &get_node()
    {
        return mNode;
    }

public:
    // todo: select from memeber, global
//    template<typename C, typename N>
//    std::enable_if_t<has_member_next<bool, const C>::value, bool> next( C &container, N &n )
//    {
//        return container.next( n );
//    }
//    template<typename C, typename N>
//    std::enable_if_t<has_global_next<bool>::value, bool> next( C &container, N &n )
//    {
//        return next( container, n );
//    }

//    template<typename C, typename N>
//    std::enable_if_t<has_member_prev<bool, C>::value, bool> prev( C &container, N &n )
//    {
//        return container.prev( n );
//    }
//    template<typename C, typename N>
//    std::enable_if_t<has_global_prev<bool>::value, bool> prev( C &container, N &n )
//    {
//        return prev( container, n );
//    }

//    template<typename C, typename N>
//    std::enable_if_t<has_member_is_end<bool, C>::value, bool> is_end( C &container, const N &n )
//    {
//        return container.is_end( n );
//    }
//    template<typename C, typename N>
//    std::enable_if_t<has_global_is_end<bool>::value, bool> is_end( C &container, const N &n )
//    {
//        return is_end( container, n );
//    }

//    template<typename C, typename N>
//    std::enable_if_t<has_member_get_value<value_type *, C>::value, bool> get_value( C &container, const N &n )
//    {
//        return container.get_value( n );
//    }
//    template<typename C, typename N>
//    std::enable_if_t<has_global_get_value<value_type *>::value, bool> get_value( C &container, const N &n )
//    {
//        return get_value( container, n );
//    }
};
}
