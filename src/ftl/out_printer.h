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
#include <iostream>
#include <memory>
#include <map>

namespace ftl
{
/// @brief container or any element that has either :
///     - c_str()
///     - OStream << T()
template<class OStream = std::ostream>
struct OutPrinter
{
    using this_type = OutPrinter;
    template<typename U = void>
    struct IsIterable
    {
        template<class T>
        static auto Check(
                typename std::enable_if<std::is_same<decltype( std::declval<T>().begin() ), decltype( std::declval<T>().end() )>::value, int>::type )
                -> decltype( bool() );
        template<class>
        static int Check( ... );
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;
    };

    template<typename U = void> // std::is_same< decltype( m.begin()->second ), decltype( m[ std::declval<decltype( m.begin()->first )> ] ) >
    struct IsMap
    {
        template<class T>
        static auto Check( typename std::enable_if<
                           std::is_same<typename std::remove_reference<decltype( std::declval<T>().begin()->second )>::type,
                                        typename std::remove_reference<decltype(
                                                std::declval<T>()[std::declval<decltype( std::declval<T>().begin()->first )>()] )>::type>::value,
                           int>::type ) -> decltype( bool() );
        template<class>
        static int Check( ... );
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;
    };
    template<typename U = void>
    struct has_c_str
    {
        template<class T>
        static auto Check( typename std::enable_if<std::is_same<const char *, decltype( std::declval<T>().c_str() )>::value, int>::type )
                -> decltype( bool() );
        template<class>
        static int Check( ... );
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;
    };
    template<typename U = void>
    struct is_like_ptr
    {
        template<class T>
        static auto Check( int ) -> decltype( *std::declval<T>(), bool() );
        template<class>
        static int Check( ... );
        static const bool value = ::std::is_same<decltype( Check<U>( 0 ) ), bool>::value;
    };
    template<typename T>
    struct is_pair : std::false_type
    {
    };
    template<typename... Ts>
    struct is_pair<std::pair<Ts...>> : std::true_type
    {
    };

    template<class T>
    struct is_specially_handled
    {
        const static bool value = IsIterable<T>::value || is_pair<T>::value || has_c_str<T>::value || is_like_ptr<T>::value;
    };

public:
    OutPrinter( std::ostream &o = std::cout ) : os( o )
    {
    }

    //------------ publicly used APIs --------------

    template<class... Tail>
    this_type &print( Tail const &... tail )
    {
        before_print();
        (void)std::initializer_list<int>{( prt( tail ), 0 )...};
        return *this;
    }
    template<class... Tail>
    this_type &println( Tail const &... tail )
    {
        before_print();
        (void)std::initializer_list<int>{( prt( tail ), 0 )...};
        os << std::endl;
        return *this;
    }
    template<class Delm, class Head, class... Tail>
    this_type &printJoined( Delm const &d, Head const &h, Tail const &... tail )
    {
        before_print();
        prt( h );
        (void)std::initializer_list<int>{( prt( d ).prt( tail ), 0 )...};
        return *this;
    }
    template<class Delm, class Head, class... Tail>
    this_type &printlnJoined( Delm const &d, Head const &h, Tail const &... tail )
    {
        before_print();
        prt( h );
        (void)std::initializer_list<int>{( prt( d ).prt( tail ), 0 )...};
        os << std::endl;
        return *this;
    }

public:
    virtual void before_print()
    {
    }
    template<class It>
    this_type &prtIter( It it0, It it1 )
    {
        if ( it0 == it1 )
            return *this;
        prt( *it0 );
        for ( ++it0; it0 != it1; ++it0 )
        {
            prt( DELM ).prt( *it0 );
        }
        return *this;
    }
    // print pair
    template<class Pair>
    typename std::enable_if<is_pair<Pair>::value, this_type &>::type prt( Pair const &tu,
                                                                          char q0 = 0,
                                                                          char q1 = 0 ) // todo: tuple like
    {
        return prt( q0 ).prt( tu.first ).prt( KVS ).prt( tu.second ).prt( q1 );
    }

    template<class T, size_t N>
    this_type &prt( T ( &t )[N] )
    {
        return prtIter( &t[0], t + N );
    }
    this_type &prt( char t )
    {
        if ( t != 0 )
            os << t;
        return *this;
    }

    this_type &prt( const char *t, bool bQuoted = false )
    {
        if ( t != nullptr )
        {
            if ( bQuoted || nInCollections )
                os << STRQ << t << STRQ;
            else
                os << t;
        }
        else
            os << "<nullptr>";
        return *this;
    }
    template<class T>
    typename std::enable_if<has_c_str<T>::value, this_type &>::type prt( const T &t )
    {
        return prt( t.c_str(), false );
    }

    template<class T>
    typename std::enable_if<IsIterable<T>::value && !IsMap<T>::value && !has_c_str<T>::value, this_type &>::type prt( const T &t )
    {
        ++nInCollections;
        prt( Q0 ).prtIter( t.begin(), t.end() ).prt( Q1 );
        --nInCollections;
        return *this;
    }
    template<class T>
    typename std::enable_if<IsMap<T>::value, this_type &>::type prt( const T &t )
    {
        ++nInCollections;
        prt( MQ0 ).prtIter( t.begin(), t.end() ).prt( MQ1 );
        --nInCollections;
        return *this;
    }
    template<class T>
    typename std::enable_if<!is_specially_handled<T>::value, this_type &>::type prt( T const &a )
    {
        os << a;
        return *this;
    }

    OStream &os;
    char DELM = ',', KVS = ':', Q0 = '[', Q1 = ']', MQ0 = '{', MQ1 = '}', STRQ = '"';
    int nInCollections = 0; // in depth of containers
};

using OSPrinter = OutPrinter<>;
static OSPrinter outPrinter;

struct ErrPrinter : OutPrinter<>
{
    ErrPrinter( std::ostream &o = std::cerr ) : OutPrinter<>( o )
    {
    }

    template<class... Tail>
    this_type &err( Tail const &... tail )
    {
        (void)std::initializer_list<int>{( prt( tail ), 0 )...};
        println( tail... );
        ++nErrors;
        return *this;
    }

    bool has_error() const
    {
        return nErrors;
    }
    void clear()
    {
        nErrors = 0;
    }

protected:
    size_t nErrors = 0;
};


struct NullPrinter
{
    using this_type = NullPrinter;
    template<class... Args>
    NullPrinter( Args &&... )
    {
    }
    template<class... Args>
    this_type &print( const Args &... )
    {
        return *this;
    }
    template<class... Args>
    this_type &println( const Args &... )
    {
        return *this;
    }
};
} // namespace ftl
