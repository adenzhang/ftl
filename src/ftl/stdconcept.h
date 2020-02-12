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
#include <tuple>
#include <map>
#include <variant>
#include <cstring>

///=============================================================
///               std library extention
///=============================================================

namespace std
{
template<class T1, class T2>
struct hash<std::pair<T1, T2>>
{
    size_t operator()( const std::pair<T1, T2> &p ) const noexcept
    {
        return std::hash<T1>()( p.first ) ^ std::hash<T2>()( p.second );
    }
};

template<class... Args>
struct hash<std::tuple<Args...>>
{
    using Tuple = std::tuple<Args...>;

    template<size_t N = 0>
    size_t operator()( const std::tuple<Args...> &t ) const noexcept
    {
        if constexpr ( N + 1 == std::tuple_size_v<Tuple> )
            return std::hash<std::tuple_element_t<N, Tuple>>()( std::get<N>( t ) );
        else
            return std::hash<std::tuple_element_t<N, Tuple>>()( std::get<N>( t ) ) ^ this->operator()<N + 1>( t );
    }
};

} // namespace std

////============================================
////              FTL_CHECK_EXPR
////============================================
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

#define FTL_CHECK_EXPR2( traitsName, ... )                                                                                                          \
    template<typename U1 = void, typename U2 = void>                                                                                                \
    struct traitsName                                                                                                                               \
    {                                                                                                                                               \
        using type = traitsName;                                                                                                                    \
                                                                                                                                                    \
        template<class T1, class T2>                                                                                                                \
        static auto Check( int ) -> decltype( __VA_ARGS__, bool() );                                                                                \
                                                                                                                                                    \
        template<class, class>                                                                                                                      \
        static int Check( ... );                                                                                                                    \
                                                                                                                                                    \
        static const bool value = ::std::is_same<decltype( Check<U1, U2>( 0 ) ), bool>::value;                                                      \
    }

#define FTL_HAS_MEMBER( name, member ) FTL_CHECK_EXPR( name, ::std::declval<T>().member )

#define FTL_IS_COMPATIBLE_FUNC_ARG( name, func ) FTL_CHECK_EXPR( name, func( ::std::declval<T>() ) )

#define FTL_IS_COMPATIBLE_FUNC_ARG_LVALUE( name, func ) FTL_CHECK_EXPR( name, func( ::std::declval<T &>() ) )

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

#define FTL_HAS_MEMBER_TYPE( name, member ) FTL_CHECK_EXPR_TYPE( name, ::std::declval<T>().member )

template<class T>
struct FTL_CHECK_TYPE_Void
{
    typedef void type;
};
#define FTL_CHECK_TYPE( traitName, ... )                                                                                                            \
    template<class T, class U = void>                                                                                                               \
    struct traitName                                                                                                                                \
    {                                                                                                                                               \
        static constexpr bool value = false;                                                                                                        \
    };                                                                                                                                              \
    template<class T>                                                                                                                               \
    struct traitName<T, typename FTL_CHECK_TYPE_Void<typename __VA_ARGS__>::type>                                                                   \
    {                                                                                                                                               \
        static constexpr bool value = true;                                                                                                         \
    }

namespace ftl
{

////=======================================================
////              template helpers
/// overload, remove_cvref_t, Identity
////=======================================================

template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overload( Ts... )->overload<Ts...>;

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class T>
struct Identity
{
    typedef T type;
};

////=======================================================
////              tuple helpers
////=======================================================

namespace tuple_utils
{
    template<size_t N, class Reduce, class Initial, class... Args>
    auto reduce( std::tuple<Args...> &&t, Reduce &&red, Initial &&initial )
    {
        using Tuple = std::tuple<Args...>;
        return red( initial, std::get<N>( t ) );
        if constexpr ( N + 1 < std::tuple_size_v<Tuple> )
            return reduce<N + 1>( std::forward<Tuple>( t ), std::forward<Reduce>( red ), red( initial, std::get<N>( t ) ) );
    }
    template<size_t N, class Reduce, class Initial, class... Args>
    auto reduce( const std::tuple<Args...> &t, Reduce &&red, Initial &&initial )
    {
        using Tuple = std::tuple<Args...>;
        return red( initial, std::get<N>( t ) );
        if constexpr ( N + 1 < std::tuple_size_v<Tuple> )
            return reduce<( N + 1 )>( t, std::forward<Reduce>( red ), red( initial, std::get<N>( t ) ) );
    }

    template<size_t N, class Callback, class... Args>
    void enumerate( std::tuple<Args...> &&t, Callback &&cb )
    {
        using Tuple = std::tuple<Args...>;
        cb( N, std::get<N>( t ) );
        if constexpr ( N + 1 <= std::tuple_size_v<Tuple> )
            enumerate<N + 1>( std::forward<Tuple>( t ), std::forward<Callback>( cb ) );
    }
    template<size_t N, class Callback, class... Args>
    void enumerate( const std::tuple<Args...> &t, Callback &&cb )
    {
        using Tuple = std::tuple<Args...>;
        cb( N, std::get<N>( t ) );
        if constexpr ( N + 1 < std::tuple_size_v<Tuple> )
            enumerate<N + 1>( std::forward<Tuple>( t ), std::forward<Callback>( cb ) );
    }

    template<size_t N, class Callback, class... Args>
    void foreach ( std::tuple<Args...> &&t, Callback && cb )
    {
        using Tuple = std::tuple<Args...>;
        cb( std::get<N>( t ) );
        if constexpr ( N + 1 < std::tuple_size_v<Tuple> )
            foreach
                <N + 1>( std::forward<Tuple>( t ), std::forward<Callback>( cb ) );
    }
    template<size_t N, class Callback, class... Args>
    void foreach ( const std::tuple<Args...> &t, Callback && cb )
    {
        using Tuple = std::tuple<Args...>;
        cb( std::get<N>( t ) );
        if constexpr ( N + 1 < std::tuple_size_v<Tuple> )
            foreach
                <N + 1>( t, std::forward<Callback>( cb ) );
    }
}; // namespace tuple_utils

struct tuple_reducer
{
    template<class Reduce, class Initial, class... Args>
    auto operator()( std::tuple<Args...> &&t, Reduce &&reduce, Initial &&initial ) const
    {
        using Tuple = std::tuple<Args...>;
        return tuple_utils::reduce<0>( std::forward<Tuple>( t ), std::forward<Reduce>( reduce ), reduce( initial, std::get<0>( t ) ) );
    }
    template<class Reduce, class Initial, class... Args>
    auto operator()( const std::tuple<Args...> &t, Reduce &&reduce, Initial &&initial ) const
    {
        using Tuple = std::tuple<Args...>;
        return tuple_utils::reduce<0>( t, std::forward<Reduce>( reduce ), reduce( initial, std::get<0>( t ) ) );
    }
};
static const tuple_reducer tuple_reduce{};

struct tuple_foreacher
{
    template<class Callback, class... Args>
    void operator()( std::tuple<Args...> &&t, Callback &&cb ) const
    {
        using Tuple = std::tuple<Args...>;
        tuple_utils::foreach<0>( std::forward<Tuple>( t ), std::forward<Callback>( cb ) );
    }
    template<class Callback, class... Args>
    void operator()( const std::tuple<Args...> &t, Callback &&cb ) const
    {
        using Tuple = std::tuple<Args...>;
        tuple_utils::foreach<0>( t, std::forward<Callback>( cb ) );
    }
};
static const tuple_foreacher tuple_foreach{};

struct tuple_enumerator
{
    template<class Callback, class... Args>
    void operator()( std::tuple<Args...> &&t, Callback &&cb ) const
    {
        using Tuple = std::tuple<Args...>;
        tuple_utils::enumerate<0>( std::forward<Tuple>( t ), std::forward<Callback>( cb ) );
    }
    template<class Callback, class... Args>
    void operator()( const std::tuple<Args...> &t, Callback &&cb ) const
    {
        using Tuple = std::tuple<Args...>;
        tuple_utils::enumerate<0>( t, std::forward<Callback>( cb ) );
    }
};
static const tuple_enumerator tuple_enumerate{};

template<typename T, typename Tuple>
struct tuple_has_type;

template<typename T, typename... Us>
struct tuple_has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...>
{
};

////=======================================================
////              variant helpers
////=======================================================

template<typename _Tp, typename _Variant, typename = void>
struct variant_accepted_index
{
    static constexpr size_t value = std::variant_npos;
};

template<typename _Tp, typename... _Types>
struct variant_accepted_index<_Tp,
                              std::variant<_Types...>,
                              decltype( std::__detail::__variant::__overload_set<_Types...>::_S_fun( std::declval<_Tp>() ), std::declval<void>() )>
{
    static constexpr size_t value =
            sizeof...( _Types ) - 1 - decltype( std::__detail::__variant::__overload_set<_Types...>::_S_fun( std::declval<_Tp>() ) )::value;
};
template<class T, class V>
struct variant_has_type
{
    static constexpr bool value = variant_accepted_index<T, V>::value != std::variant_npos;
};

////=======================================================
////              array helpers
////=======================================================

template<class T, size_t N>
constexpr size_t array_size( T[N] )
{
    return N;
}
//=============================================
//   GetClassName
//=============================================
namespace internal
{

    template<typename T>
    struct GetTypeNameHelper
    {
        static constexpr const unsigned int BACK_SIZE = sizeof( "]" ) - 1u;

        static const char *GetTypeNameRaw( void )
        {
            constexpr unsigned int FRONT_SIZE = sizeof( "static const char* internal::GetTypeNameHelper<T>::GetTypeNameRaw() [with T = " ) - 1u;
            auto pName = __PRETTY_FUNCTION__;
            return pName + FRONT_SIZE;
        }

        static const char *GetTypeName( char *buf, int *len )
        {
            constexpr unsigned int FRONT_SIZE =
                    sizeof( "static const char* internal::GetTypeNameHelper<T>::GetTypeName(char*, int*) [with T = " ) - 1u;
            const char *pName = __PRETTY_FUNCTION__ + FRONT_SIZE;
            int Len = int( strlen( pName ) - BACK_SIZE );
            if ( len )
            {
                Len = std::min( Len, *len );
                *len = Len;
            }

            memcpy( buf, pName, Len );
            buf[Len] = 0;
            return buf;
        }
    };
} // namespace internal

// len [in]: buf capacity
//     [out]: strlen populated
template<typename T>
const char *GetTypeName( char *buf, int *len = nullptr )
{
    return internal::GetTypeNameHelper<T>::GetTypeName( buf, len );
}

template<typename T>
std::string GetTypeNameString( void )
{
    auto s = internal::GetTypeNameHelper<T>::GetTypeNameRaw();
    return std::string( s, strlen( s ) - internal::GetTypeNameHelper<T>::BACK_SIZE );
}

////=======================================================
////              macros for creating containers with initial values
/// E.g     auto vec = VEC_T( int, 1, 2, 3, 5 );
///    auto m = MAP_T( int, int, {1, 2}, {3, 5} );
///
////=======================================================

#define VEC_T( type, ... )                                                                                                                          \
    std::vector<type>                                                                                                                               \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

#define LIST_T( type, ... )                                                                                                                         \
    std::list<type>                                                                                                                                 \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

#define SET_T( type, ... )                                                                                                                          \
    std::set<type>                                                                                                                                  \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

#define HSET_T( type, ... )                                                                                                                         \
    std::unordered_set<type>                                                                                                                        \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

#define MAP_T( Ktype, Vtype, ... )                                                                                                                  \
    std::map<Ktype, Vtype>                                                                                                                          \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

#define HMAP_T( Ktype, Vtype, ... )                                                                                                                 \
    std::unordered_map<Ktype, VType>                                                                                                                \
    {                                                                                                                                               \
        __VA_ARGS__                                                                                                                                 \
    }

} // namespace ftl
