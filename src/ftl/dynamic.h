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

#include <cassert>
#include <ftl/stdconcept.h> // overload
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>
#include <ostream>
#include <fstream>

namespace ftl
{


template<class E, class... PrimitiveTypes>
class dynamic_primitives;

/// @tparam K Key type of dynamic type.
/// @tparam PrimitiveTypes..., all the primitive types.
template<class K, class... PrimitiveTypes>
class dynamic_var;


template<class K, class... PrimitiveTypes>
using dynamic_ptr = std::unique_ptr<dynamic_var<K, PrimitiveTypes...>>;

template<class K, class... PrimitiveTypes>
using dynamic_vec = std::vector<dynamic_ptr<K, PrimitiveTypes...>>;

/// @class dynamic_map should be defined as OrderedDict to meet some requirement.
template<class K, class... PrimitiveTypes>
using dynamic_map = std::unordered_map<K, dynamic_ptr<K, PrimitiveTypes...>>;

template<class K, class... PrimitiveTypes>
using dynamic_var_ = std::variant<dynamic_map<K, PrimitiveTypes...>, PrimitiveTypes..., dynamic_vec<K, PrimitiveTypes...>>;


/// @brief Generaic make_dynamic create dynamic_var D.
/// Specilization dynamic_vec or dynamic_map.
/// @tparam D dynamic_var type
/// @tparam ET dynamic type: dynamic_map, dynamic_vec, or any primitive type.
template<class D, class ET, class... Args>
D make_dynamic( Args &&... args )
{
    if constexpr ( std::is_same_v<D, ET> )
    { // copy construct
        return D( std::forward<Args>( args )... );
    }
    else
        return D( std::in_place_type_t<std::remove_cv_t<std::remove_reference_t<ET>>>(), std::forward<Args>( args )... );
}


template<class K, class... PrimitiveTypes>
class dynamic_var : public dynamic_var_<K, PrimitiveTypes...>
{
public:
    using base_type = dynamic_var_<K, PrimitiveTypes...>;
    using this_type = dynamic_var<K, PrimitiveTypes...>;

    using key_type = K;
    using vec_type = dynamic_vec<K, PrimitiveTypes...>;
    using map_type = dynamic_map<K, PrimitiveTypes...>;

    static constexpr auto type_index_size = sizeof...( PrimitiveTypes ) + 2; // plus map, vec.
    static constexpr auto map_type_index = 0; // first type is map.
    static constexpr auto str_type_index = 1; // str type, index == 1.
    static constexpr auto vec_type_index = type_index_size - 1; // last type is vec.

    template<size_t n, typename Head, typename... Tail>
    struct NthTypeImpl
    {
        using type = typename NthTypeImpl<n - 1, Tail...>::type;
    };

    template<typename Head, typename... Tail>
    struct NthTypeImpl<0, Head, Tail...>
    {
        using type = Head;
    };
    template<size_t N>
    using NthType = typename NthTypeImpl<N, dynamic_map<K, PrimitiveTypes...>, PrimitiveTypes..., dynamic_vec<K, PrimitiveTypes...>>::type;

    using str_type = NthType<str_type_index>;

    dynamic_var() = default;

    template<typename... Ts>
    dynamic_var( Ts &&... xs ) : base_type{std::forward<Ts>( xs )...}
    {
    }

    dynamic_var( this_type &&another ) = default;

    dynamic_var( const this_type &another )
    {
        as_variant() = std::move( another.clone().as_variant() );
    }

    this_type &operator=( this_type &&another ) = default;
    this_type &operator=( const this_type &another )
    {
        as_variant() = std::move( another.clone().as_variant() );
        return *this;
    }

    template<class ET, class... Args>
    auto &emplace( Args &&... args )
    {
        return as_variant().template emplace<ET>( std::forward<Args>( args )... );
    }

    /// @brief Deep copy.
    this_type clone() const
    {
        return accept( overload{[]( const map_type &e ) {
                                    auto r = make_dynamic<this_type, map_type>();
                                    auto &m = r.template as<map_type>();
                                    for ( const auto &p : e )
                                    {
                                        assert( p.second );
                                        auto newV = p.second->clone();
                                        m.emplace( p.first, make_dynamic_ptr( std::move( newV ) ) );
                                    }
                                    return r;
                                },
                                []( const vec_type &e ) {
                                    auto r = make_dynamic<this_type, vec_type>();
                                    auto &m = r.template as<vec_type>();
                                    for ( const auto &p : e )
                                    {
                                        assert( p );
                                        auto newV = p->clone();
                                        m.emplace_back( make_dynamic_ptr( std::move( newV ) ) );
                                    }
                                    return r;
                                },
                                []( const auto &e ) {
                                    using T = std::remove_cv_t<decltype( e )>;
                                    return make_dynamic<this_type, T>( e );
                                }} );
    }


    base_type &as_variant()
    {
        return *this;
    }
    const base_type &as_variant() const
    {
        return *this;
    }

    template<size_t n>
    bool is_nth_type() const
    {
        return base_type::index() == n;
    }
    template<class ET>
    ET &as()
    {
        return std::get<ET>( *this );
    }
    template<class ET>
    const ET &as() const
    {
        return std::get<ET>( *this );
    }

    bool is_map() const
    {
        return is_nth_type<map_type_index>();
    }
    bool is_vec() const
    {
        return is_nth_type<vec_type_index>();
    }
    const auto &as_str() const
    {
        return as<NthType<str_type_index>>();
    }

    // call func(std::string& key, dynamic_var &);
    template<class Func>
    void foreach_map_entry( Func &&func ) const
    {
        accept( overload{[&]( const map_type &e ) {
                             for ( const auto &p : e ) // clang-format doesn't work with const auto& [n,v] = p;
                                 func( p.first, *p.second ); // type of p.second == std::unique_ptr<dynamic_var> or dynamic_ptr. NOTE: nullptr
                         },
                         [&]( const vec_type & ) {},
                         [&]( const auto & ) {}} );
    }
    // call func( dynamic_var &);
    template<class Func>
    void foreach_vec_entry( Func &&func ) const
    {
        accept( overload{[&]( const map_type & ) {},
                         [&]( const vec_type &e ) {
                             for ( const auto &v : e )
                             {
                                 func( *v ); // type of v == std::unique_ptr<dynamic_var> or dynamic_ptr
                             }
                         },
                         [&]( const auto & ) {}} );
    }
    template<class Visitor>
    constexpr decltype( auto ) accept( Visitor &&visitor )
    {
        return std::visit( std::forward<Visitor>( visitor ), as_variant() );
    }
    template<class Visitor>
    constexpr decltype( auto ) accept( Visitor &&visitor ) const
    {
        return std::visit( std::forward<Visitor>( visitor ), as_variant() );
    }

    void print_json( std::ostream &os, const char *quote = "\"" ) const
    {
        accept( overload{[&]( const map_type &e ) {
                             os << "{ ";
                             int k = 0;
                             for ( const auto &p : e ) // clang-format doesn't work with const auto& [n,v] = p;
                             {
                                 if ( ++k != 1 )
                                     os << ", ";
                                 os << p.first << ": ";
                                 p.second->print_json( os );
                             }
                             os << " }";
                         },
                         [&]( const vec_type &e ) {
                             os << "[ ";
                             int k = 0;
                             for ( const auto &v : e )
                             {
                                 if ( ++k != 1 )
                                     os << ", ";
                                 v->print_json( os );
                             }
                             os << " ]";
                         },
                         [&]( const auto &e ) {
                             if ( quote )
                                 os << quote;
                             os << e;
                             if ( quote )
                                 os << quote;
                         }} );
    }

    /// if K already exists, return the old value.
    /// throws if it's not a map
    /// @return ref to new added element.
    template<class ET, class... Args>
    this_type &map_add( const K &k, Args &&... args )
    {
        auto &p = as<map_type>();
        return *p.emplace( k, make_dynamic_ptr( make_dynamic<this_type, ET>( std::forward<Args>( args )... ) ) ).first->second;
    }
    this_type &map_add_entry( const K &k, dynamic_var var )
    {
        auto &p = as<map_type>();
        return *p.emplace( k, make_dynamic_ptr( std::move( var ) ) ).first->second;
    }

    /// throws if it's not a vec
    /// @return ref to new added element.
    template<class ET, class... Args>
    this_type &vec_add( Args &&... args )
    {
        auto &p = as<vec_type>();
        return *p.emplace_back( make_dynamic_ptr( make_dynamic<this_type, ET>( std::forward<Args>( args )... ) ) );
    }
    this_type &vec_add_entry( dynamic_var var )
    {
        auto &p = as<vec_type>();
        return *p.emplace_back( make_dynamic_ptr( std::move( var ) ) );
    }
};

template<class K, class... PrimitiveTypes>
std::ostream &operator<<( std::ostream &os, const dynamic_var<K, PrimitiveTypes...> &var )
{
    var.print_json( os );
    return os;
}

template<class K, class E, class... Args>
dynamic_ptr<K, E, Args...> make_dynamic_ptr( dynamic_var<K, E, Args...> &&v )
{
    return std::make_unique<dynamic_var<K, E, Args...>>( std::move( v ) );
}

// return std::pair<iterator, bool>
template<class V, class N, class K, class E, class... Args>
auto dynamic_map_add( dynamic_var<K, E, Args...> &d, const N &n, V &&v )
{
    static_assert( std::is_constructible_v<K, const N>, "Unable to construct K from N" );

    using DT = dynamic_var<K, E, Args...>;
    using DM = dynamic_map<K, E, Args...>;
    auto &p = d.template as<DM>();
    {
        if constexpr ( std::is_same_v<DT, V> )
            return p.emplace( n, make_dynamic_ptr( std::move( v ) ) );
        else // V is a primitive type
            return p.emplace( n, make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
    }
}
// return std::pair<iterator, bool>
template<class V, class N, class K, class E, class... Args>
auto dynamic_map_add( dynamic_map<K, E, Args...> &d, const N &n, V &&v )
{
    static_assert( std::is_constructible_v<K, const N>, "Unable to construct K from N" );
    // V must be in Args...

    using DT = dynamic_var<K, E, Args...>;
    using DM = dynamic_map<K, E, Args...>;
    if constexpr ( std::is_same_v<DT, V> )
        return d.emplace( n, make_dynamic_ptr( std::move( v ) ) );
    else
        return d.emplace( n, make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
}

template<class V, class K, class E, class... Args>
auto &dynamic_vec_add( dynamic_var<K, E, Args...> &d, V &&v )
{
    using DT = dynamic_var<K, E, Args...>;
    using DA = dynamic_vec<K, E, Args...>;
    auto &p = d.template as<DA>();
    if constexpr ( std::is_same_v<DT, V> )
        return p.emplace_back( make_dynamic_ptr( std::move( v ) ) );
    else
        return p.emplace_back( make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
}

template<class V, class K, class E, class... Args>
dynamic_vec<K, E, Args...> *dynamic_vec_add( dynamic_vec<K, E, Args...> &d, V &&v )
{
    using DT = dynamic_var<K, E, Args...>;

    if constexpr ( std::is_same_v<DT, V> )
        d.emplace_back( make_dynamic_ptr( std::move( v ) ) );
    else
        d.emplace_back( make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
    return &d;
}

///////////////////////////////////////////////////////////////////////////////////
///     dynamic_var read from string
///////////////////////////////////////////////////////////////////////////////////

// return -1 for EOF
// 0 for error
// 1 for string
// other for grammar
inline int read_json_str( std::istream &ss, std::string &s, std::ostream &err )
{
    struct my
    {
        static bool is_quote( int c )
        {
            return c == '\'' || c == '\"';
        }
        static bool is_grammar_char( int c )
        {
            return c == ',' || c == ':' || c == '[' || c == ']' || c == '{' || c == '}';
        }
        static bool is_valid_quoted_char( int c )
        {
            return isalnum( c ) || isspace( c );
        }
        static int skipUntilChar( std::istream &ss )
        {
            while ( ss )
            {
                auto c = ss.get();
                if ( is_grammar_char( c ) || isalnum( c ) || is_quote( c ) )
                    return c;
                assert( isspace( c ) );
            }
            return EOF;
        }
    };

    s.clear();
    auto c = my::skipUntilChar( ss );
    if ( c == EOF )
    {
        s = "EOF";
        return -1;
    }
    if ( my::is_grammar_char( c ) )
    {
        s += char( c );
        return c;
    }
    if ( my::is_quote( c ) )
    {
        char q = char( c );
        while ( ss )
        {
            c = ss.get();
            if ( c == q ) // quote ends
            {
                return 1; // got string
            }
            if ( c == '\\' ) // escape
            {
                if ( ss )
                {
                    auto cc = ss.get();
                    if ( cc == '\"' || cc == '\'' )
                        s += char( cc );
                    else
                    {
                        err << "Unable to escape " << char( cc );
                        return 0;
                    }
                }
                else
                {
                    err << "EOF when expecting quote: " << q;
                    return 0;
                }
            }
            else // shoud error when see \n .
            {
                s += char( c );
            }
        }
    }
    else // isalnum, unquoted string
    {
        s += char( c );
        while ( ss )
        {
            c = ss.peek();
            if ( isalnum( c ) )
            {
                s += char( ss.get() );
            }
            else if ( my::is_grammar_char( c ) || std::isspace( c ) )
                break;
            else
            {
                err << " Illegal char at end of string:" << s << ", char:" << c;
                return 0;
            }
        }
        return 1; // got string
    }
    assert( false );
    return 0; // never reach here
}

// 0 for error
// other for grammar
template<class K, class Str>
bool dynamic_read_json( dynamic_var<K, Str> &dyn, std::istream &ss, std::ostream &err )
{
    using DynVar = dynamic_var<K, Str>;
    using DynMap = typename dynamic_var<K, Str>::map_type;
    using DynVec = typename dynamic_var<K, Str>::vec_type;
    using DynStr = typename dynamic_var<K, Str>::str_type;

    std::string s;
    auto res = read_json_str( ss, s, err );
    if ( res == '{' ) // read map
    {
        dyn = make_dynamic<DynVar, DynMap>();
        while ( true )
        {
            std::string key;
            res = read_json_str( ss, key, err );
            if ( res == '}' ) // end of map
                return true;
            if ( res == ',' )
                continue;
            if ( res != 1 )
            {
                err << " | expecting map key but got " << key;
                return false;
            }
            //-- now have key already
            res = read_json_str( ss, s, err );
            if ( res != ':' )
            {
                err << " | expecting : but got" << s;
                return false;
            }
            DynVar child;
            auto r = dynamic_read_json( child, ss, err );
            if ( !r )
            {
                err << " | error reading value for key=" << key;
                return false;
            }
            dyn.map_add_entry( Str( key ), std::move( child ) );
        }
    }
    else if ( res == '[' ) // read vec
    {
        dyn = make_dynamic<DynVar, DynVec>();
        while ( true )
        {
            DynVar child;
            auto r = dynamic_read_json( child, ss, err );
            if ( !r )
            {
                err << " | error reading vec value";
                return false;
            }
            dyn.vec_add_entry( std::move( child ) );
            res = read_json_str( ss, s, err );
            if ( res == ']' )
                return true;
            if ( res == ',' )
                continue;
        }
    }
    else if ( res == 1 ) // read string
    {
        dyn = make_dynamic<DynVar, DynStr>( s );
        return true;
    }
    else
    {
        err << " | expecting map or vec or string.";
        return false;
    }
}
} // namespace ftl
