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

namespace ftl
{
template<class K, class E, class... Args>
struct dynamic_wrapper;

template<class K, class E, class... Args>
using dynamic_ptr = std::unique_ptr<dynamic_wrapper<K, E, Args...>>;
using dynamic_string = std::string;
template<class K, class E, class... Args>
using dynamic_array = std::vector<dynamic_ptr<K, E, Args...>>;
template<class K, class E, class... Args>
using dynamic_map = std::unordered_map<K, dynamic_ptr<K, E, Args...>>;

template<class K, class E, class... Args>
using dynamic_var = std::variant<dynamic_array<K, E, Args...>, dynamic_map<K, E, Args...>, E, Args...>;

template<class K, class E, class... Args>
struct dynamic_wrapper
{
    dynamic_var<K, E, Args...> data;

    template<typename... Ts>
    dynamic_wrapper( Ts &&... xs ) : data{std::forward<Ts>( xs )...}
    {
    }
};

template<class K, class E, class... Args>
dynamic_ptr<K, E, Args...> make_dynamic_ptr( dynamic_var<K, E, Args...> &&v )
{
    return std::make_unique<dynamic_wrapper<K, E, Args...>>( std::move( v ) );
}

// map or array
template<class D, class T, class... A>
std::enable_if_t<variant_accepted_index<T, D>::value == 1 || variant_accepted_index<T, D>::value == 0, D> make_dynamic()
{
    return D( std::in_place_type_t<std::remove_cv_t<std::remove_reference_t<T>>>() );
}

template<class D, class T, class... A>
std::enable_if_t<variant_accepted_index<T, D>::value != 1 || variant_accepted_index<T, D>::value != 0 ||
                         variant_accepted_index<T, D>::value != std::variant_npos,
                 D>
make_dynamic( A &&... a )
{
    return D( std::in_place_type_t<std::remove_cv_t<std::remove_reference_t<T>>>(), std::forward<A>( a )... );
}

template<class V, class N, class K, class E, class... Args>
dynamic_map<K, E, Args...> *add_dynamic_map( dynamic_var<K, E, Args...> &d, const N &n, V &&v )
{
    static_assert( std::is_constructible_v<K, const N>, "Unable to construct K from N" );

    using DT = dynamic_var<K, E, Args...>;
    using DM = dynamic_map<K, E, Args...>;
    if ( auto p = std::get_if<DM>( &d ) )
    {
        if constexpr ( std::is_same_v<DT, V> )
            p->emplace( n, make_dynamic_ptr( std::move( v ) ) );
        else // V is an atomic type
            p->emplace( n, make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
        return p;
    }
    else
    {
        assert( false && "Unable to add elment to non-map" );
    }

    return nullptr;
}
template<class V, class N, class K, class E, class... Args>
dynamic_map<K, E, Args...> *add_dynamic_map( dynamic_map<K, E, Args...> &d, const N &n, V &&v )
{
    static_assert( std::is_constructible_v<K, const N>, "Unable to construct K from N" );

    using DT = dynamic_var<K, E, Args...>;
    using DM = dynamic_map<K, E, Args...>;
    if constexpr ( std::is_same_v<DT, V> )
        d.emplace( n, make_dynamic_ptr( std::move( v ) ) );
    else
        d.emplace( n, make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
    return &d;
}

template<class V, class K, class E, class... Args>
dynamic_array<K, E, Args...> *add_dynamic_array( dynamic_var<K, E, Args...> &d, V &&v )
{
    using DT = dynamic_var<K, E, Args...>;
    using DA = dynamic_array<K, E, Args...>;
    if ( auto p = std::get_if<DA>( &d ) )
    {
        if constexpr ( std::is_same_v<DT, V> )
            p->emplace_back( make_dynamic_ptr( std::move( v ) ) );
        else
            p->emplace_back( make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
        return p;
    }
    else
    {
        assert( false && "Unable to add elment to non-array" );
    }
    return nullptr;
}

template<class V, class K, class E, class... Args>
dynamic_array<K, E, Args...> *add_dynamic_array( dynamic_array<K, E, Args...> &d, V &&v )
{
    using DT = dynamic_var<K, E, Args...>;

    if constexpr ( std::is_same_v<DT, V> )
        d.emplace_back( make_dynamic_ptr( std::move( v ) ) );
    else
        d.emplace_back( make_dynamic_ptr( make_dynamic<DT, V>( std::forward<V>( v ) ) ) );
    return &d;
}

template<class K, class E, class... Args>
dynamic_var<K, E, Args...> deep_copy( const dynamic_var<K, E, Args...> &d )
{
    using DT = dynamic_var<K, E, Args...>;
    using DM = dynamic_map<K, E, Args...>;
    using DA = dynamic_array<K, E, Args...>;

    return std::visit( overload{[]( const DM &e ) {
                                    auto r = make_dynamic<DT, DM>();
                                    auto &m = std::get<DM>( r );
                                    for ( const auto &[n, v] : e )
                                    {
                                        assert( v );
                                        auto newV = deep_copy( v->data );
                                        m.emplace( n, make_dynamic_ptr( std::move( newV ) ) );
                                    }
                                    return r;
                                },
                                []( const DA &e ) {
                                    auto r = make_dynamic<DT, DA>();
                                    auto &m = std::get<DA>( r );
                                    for ( const auto &v : e )
                                    {
                                        assert( v );
                                        auto newV = deep_copy( v->data );
                                        m.emplace_back( make_dynamic_ptr( std::move( newV ) ) );
                                    }
                                    return r;
                                },
                                []( const auto &e ) {
                                    using T = std::remove_cv_t<decltype( e )>;
                                    auto r = make_dynamic<DT, T>( e );
                                    return r;
                                }},
                       d );

    assert( false );
}
} // namespace ftl
