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
#include <string_view>
#include <array>

/*****************************************************
  usage:
#define Fruit_( T ) T( None, '0' ), T( Apple, '1' ), T( Pear, '2' ), T( Max_, '2' + 1 )
CREATE_ENUM( Fruit, char )

*******************************************************/
namespace ftl
{
namespace internal
{
    // enum_data must be initialized from literal or static string
    template<typename IdType>
    struct enum_data
    {
        enum_data()
        {
        }
        enum_data( IdType _id, const char *_pszName ) : id( _id ), name( _pszName )
        {
        }

        IdType id;
        std::string_view name;
    };

} // namespace internal
} // namespace ftl


#define MAKE_ENUM_TWO( name, id ) name = id

#define MAKE_CONVERT_TWO( name, id )                                                                                                                \
    {                                                                                                                                               \
        id, #name                                                                                                                                   \
    }

#define CREATE_ENUM( N, type )                                                                                                                      \
    enum class N : type                                                                                                                             \
    {                                                                                                                                               \
        N##_( MAKE_ENUM_TWO )                                                                                                                       \
    };                                                                                                                                              \
                                                                                                                                                    \
    struct N##StringArray                                                                                                                           \
    {                                                                                                                                               \
        N##StringArray()                                                                                                                            \
        {                                                                                                                                           \
            ftl::internal::enum_data<type> d[] = {N##_( MAKE_CONVERT_TWO )};                                                                        \
            for ( std::size_t i = 0, N = sizeof( d ) / sizeof( ftl::internal::enum_data<type> ); i < N; ++i )                                       \
            {                                                                                                                                       \
                auto id = d[i].id;                                                                                                                  \
                if ( in_range( id, value.size() ) )                                                                                                 \
                    value[static_cast<size_t>( id )].swap( d[i].name );                                                                             \
            }                                                                                                                                       \
        }                                                                                                                                           \
        template<typename T, typename U>                                                                                                            \
        std::enable_if_t<std::is_unsigned<T>::value && std::is_unsigned<U>::value, bool> in_range( const T &x, const U &y )                         \
        {                                                                                                                                           \
            return x < y;                                                                                                                           \
        }                                                                                                                                           \
        template<typename T, typename U>                                                                                                            \
        std::enable_if_t<!std::is_unsigned<T>::value && std::is_unsigned<U>::value, bool> in_range( const T &x, const U &y )                        \
        {                                                                                                                                           \
            return x >= 0 && static_cast<U>( x ) < y;                                                                                               \
        }                                                                                                                                           \
                                                                                                                                                    \
        std::array<::std::string_view, size_t( N::Max_ )> value;                                                                                    \
    };                                                                                                                                              \
    const N##StringArray arr##N##StringArray;                                                                                                       \
                                                                                                                                                    \
    inline const char *Get##N##Name( N ct )                                                                                                         \
    {                                                                                                                                               \
        std::size_t id = size_t( ct );                                                                                                              \
        if ( id < arr##N##StringArray.value.size() )                                                                                                \
            return arr##N##StringArray.value[id].data();                                                                                            \
        return nullptr;                                                                                                                             \
    }                                                                                                                                               \
                                                                                                                                                    \
    inline N Get##N( const char *pszCt )                                                                                                            \
    {                                                                                                                                               \
        for ( std::size_t id = 0; id < arr##N##StringArray.value.size(); ++id )                                                                     \
        {                                                                                                                                           \
            auto name = arr##N##StringArray.value[id];                                                                                              \
            if ( name.length() == strlen( pszCt ) && strncasecmp( pszCt, name.data(), name.length() ) == 0 )                                        \
                return N( id );                                                                                                                     \
        }                                                                                                                                           \
                                                                                                                                                    \
        return N::None;                                                                                                                             \
    }                                                                                                                                               \
                                                                                                                                                    \
    inline const char *to_cstr( N e, const char *defaultValue = nullptr )                                                                           \
    {                                                                                                                                               \
        std::size_t id = size_t( e );                                                                                                               \
        const char *result = id < arr##N##StringArray.value.size() ? arr##N##StringArray.value[id].data() : nullptr;                                \
        return result ? result : defaultValue;                                                                                                      \
    }                                                                                                                                               \
                                                                                                                                                    \
    inline std::string_view to_string_view( N e )                                                                                                   \
    {                                                                                                                                               \
        std::size_t id = size_t( e );                                                                                                               \
        if ( id < arr##N##StringArray.value.size() )                                                                                                \
            return arr##N##StringArray.value[id];                                                                                                   \
        return {};                                                                                                                                  \
    }                                                                                                                                               \
                                                                                                                                                    \
    inline bool str_to( ::std::string_view s, N &e )                                                                                                \
    {                                                                                                                                               \
        if ( !s.length() )                                                                                                                          \
            return false;                                                                                                                           \
        for ( std::size_t id = 0; id < arr##N##StringArray.value.size(); ++id )                                                                     \
        {                                                                                                                                           \
            auto name = arr##N##StringArray.value[id];                                                                                              \
            if ( name.length() == s.length() && strncasecmp( s.data(), name.data(), name.length() ) == 0 )                                          \
            {                                                                                                                                       \
                e = N( id );                                                                                                                        \
                return true;                                                                                                                        \
            }                                                                                                                                       \
        }                                                                                                                                           \
        return false;                                                                                                                               \
    }                                                                                                                                               \
                                                                                                                                                    \
                                                                                                                                                    \
    inline ::std::ostream &operator<<( ::std::ostream &os, N e )                                                                                    \
    {                                                                                                                                               \
        os << ( static_cast<std::underlying_type_t<N>>( e ) == 0 ? '0' : static_cast<std::underlying_type_t<N>>( e ) ) << '('                       \
           << to_cstr( e, "<unknown>" ) << ')';                                                                                                     \
        return os;                                                                                                                                  \
    }


//inline ::std::optional<N> str_to(::ftl::Identity<N>, ::std::string_view s )                                                                     \
//{                                                                                                                                               \
//    if ( !s.length() )                                                                                                                          \
//        return {};                                                                                                                              \
//    for ( std::size_t id = 0; id < arr##N##StringArray.value.size(); ++id )                                                                     \
//    {                                                                                                                                           \
//        auto name = arr##N##StringArray.value[id];                                                                                              \
//        if ( name.length() == s.length() && strncasecmp( s.data(), name.data(), name.length() ) == 0 )                                          \
//        {                                                                                                                                       \
//            return {static_cast<N>( id )};                                                                                                      \
//        }                                                                                                                                       \
//    }                                                                                                                                           \
//    return {};                                                                                                                                  \
//}                                                                                                                                               \

