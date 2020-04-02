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



#ifndef CONTAINERSERIALIZATION_H_
#define CONTAINERSERIALIZATION_H_
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>
#include <fstream>

namespace ftl
{

struct StrLiteral
{
    const char *data = nullptr;
    size_t len = 0;
};

constexpr StrLiteral operator""_sl( const char *s, size_t n )
{
    return {s, n};
}

/******************************
   scoped_stream_redirect & scoped_stream_redirect_to_file.
   Usage:
{
    using namespace ftl;
    MinScalarProduct sln;
    SECTION( "basic" )
    {
        auto redirectin = ftl::make_scoped_istream_redirect( std::cin, R"(2
                                                                 3
                                                                 1 3 -5
                                                                 -2 4 1
                                                                 5
                                                                 1 2 3 4 5
                                                                 1 0 1 0 1
                                                                 )"_sl );
        sln.main();
    }
    SECTION( "from small file" )
    {
        scoped_stream_redirect_to_file redirectin( std::cin, "data/A-small-practice.in", std::fstream::in );
        scoped_stream_redirect_to_file redirectout( std::cout, "data/A-small-practice.out", std::fstream::out );
        sln.main();
    }
}
 * **********************************************************/
template<class IOType, class TempIO>
struct scoped_stream_redirect
{
    using stream_type = TempIO;
    stream_type ss;
    std::streambuf *rdbuf = nullptr; // original rdbuf
    IOType &io; // stream to redirect

    scoped_stream_redirect() = delete;
    scoped_stream_redirect( const scoped_stream_redirect & ) = delete;
    scoped_stream_redirect( scoped_stream_redirect &&a ) : ss( std::move( a.ss ) ), rdbuf( a.rdbuf ), io( a.io )
    {
        a.rdbuf = nullptr;
    }

    // temp ss is not used when another is provided.
    scoped_stream_redirect( IOType &io, TempIO &another ) : rdbuf( io.rdbuf() ), io( io )
    {
        io.rdbuf( another.rdbuf() );
    }
    scoped_stream_redirect( IOType &io, const char *s ) : ss( s ), rdbuf( io.rdbuf() ), io( io )
    {
        io.rdbuf( ss.rdbuf() );
    }
    ~scoped_stream_redirect()
    {
        if ( rdbuf )
        {
            io.rdbuf( rdbuf );
            rdbuf = nullptr;
        }
    }
};

/// redirect to file
template<class IOType>
struct scoped_stream_redirect_to_file
{
    //    std::ifstream ifs;
    std::fstream fst;
    std::streambuf *rdbuf = nullptr; // original rdbuf
    IOType &io; // stream to redirect

    scoped_stream_redirect_to_file( IOType &io, const char *filename, std::ios_base::openmode mode ) : io( io )
    {
        fst.open( filename, mode );
        if ( fst.is_open() )
        {
            rdbuf = io.rdbuf();
            io.rdbuf( fst.rdbuf() );
        }
        else
        {
            rdbuf = nullptr;
            throw std::runtime_error( std::string( "failed to open file " ) + filename );
        }
    }
    ~scoped_stream_redirect_to_file()
    {
        if ( rdbuf )
        {
            io.rdbuf( rdbuf );
            rdbuf = nullptr;
        }
    }
};

template<class IOType, class TempIO, typename = std::enable_if_t<!std::is_constructible_v<TempIO, const char *>, void>>
scoped_stream_redirect<IOType, TempIO> make_scoped_stream_redirect( IOType &io, TempIO &ss )
{
    return {io, ss};
};

template<class IOType, class T, typename = std::enable_if_t<std::is_constructible_v<T, const char>, void>>
scoped_stream_redirect<IOType, std::stringstream> make_scoped_istream_redirect( IOType &io, T *str )
{
    return {io, static_cast<const char *>( str )};
};
template<class IOType, class T, size_t N, typename = std::enable_if_t<std::is_constructible_v<T, const char>, void>>
scoped_stream_redirect<IOType, std::stringstream> make_scoped_istream_redirect( IOType &io, T str[N] )
{
    return {io, static_cast<const char *>( str )};
};

template<class IOType, class T, typename = std::enable_if_t<std::is_same_v<T, StrLiteral>, void>>
scoped_stream_redirect<IOType, std::stringstream> make_scoped_istream_redirect( IOType &io, T str )
{
    return {io, static_cast<const char *>( str.data )};
};

namespace serialization
{

    /// @brief print quoted string
    inline std::ostream &operator<<( std::ostream &os, const std::string &s )
    {
        return os << '\"' << s.c_str() << '\"';
    }
    template<typename Iter>
    std::ostream &printIterator( std::ostream &os, Iter itBegin, Iter itEnd, const char sep = ',', const char *brackets = "[]" )
    {
        if ( brackets && brackets[0] )
            os << brackets[0];
        for ( auto it = itBegin; it != itEnd; ++it )
        {
            if ( it == itBegin )
            {
                os << *it;
            }
            else
            {
                os << sep << *it;
            }
        }
        if ( brackets && brackets[1] )
            os << brackets[1];
        return os;
    }
    template<typename Map>
    std::ostream &printMap( std::ostream &os, const Map &v, const char sep = ',', const char kvsep = ':', const char *brackets = "{}" )
    {
        if ( brackets && brackets[0] )
            os << brackets[0];
        for ( typename Map::const_iterator it2 = v.begin(); it2 != v.end(); ++it2 )
        {
            if ( it2 == v.begin() )
            {
                os << it2->first << kvsep << it2->second;
            }
            else
            {
                os << sep << it2->first << kvsep << it2->second;
            }
        }
        if ( brackets && brackets[0] )
            os << brackets[0];
        return os;
    }
    template<typename Key, typename Value>
    std::ostream &operator<<( std::ostream &os, const std::map<Key, Value> &v )
    {
        return printMap( os, v );
    }
    template<typename Key, typename Value>
    std::ostream &operator<<( std::ostream &os, const std::unordered_map<Key, Value> &v )
    {
        return printMap( os, v );
    }
    template<typename Elm>
    std::ostream &operator<<( std::ostream &os, const std::vector<Elm> &v )
    {
        return printIterator( os, v.begin(), v.end() );
    }
    template<typename Elm>
    std::ostream &operator<<( std::ostream &os, const std::list<Elm> &v )
    {
        return printIterator( os, v.begin(), v.end() );
    }
    template<typename Elm>
    std::ostream &operator<<( std::ostream &os, const std::set<Elm> &v )
    {
        return printIterator( os, v.begin(), v.end() );
    }

    //=========== read ============================
    // return number of chars skipped
    inline int skipSpace( std::istream &is )
    {
        int count = 0;
        for ( ; std::isspace( is.peek() ); ++count, is.ignore() )
            ;
        return count;
    }
    inline std::istream &operator>>( std::istream &is, std::string &s )
    {
        skipSpace( is );
        if ( '\"' != is.peek() )
            return is;
        is.ignore();
        int prev = 0;
        while ( is )
        {
            int c = is.get();
            if ( '\"' == c && '\"' != prev )
            {
                return is;
            }
            else if ( EOF == c )
            {
                return is;
            }
            else
            {
                s += char( c );
                prev = c;
            }
        }
        return is;
    }
    // return number of elements; -1 for error
    // quotes : format "[]"; default "[]" or "{}"
    template<typename Iter>
    int readiterator( std::istream &is, Iter it, char sep = ',', const char *quotes = nullptr )
    {
        enum TOK
        {
            BRACKET0,
            BRACKET1,
            ELEM,
            SEP
        };
        skipSpace( is );
        char foundQuote = 0;
        if ( quotes && quotes[0] )
        {
            char c = is.get();
            if ( quotes[0] != c )
            {
                std::cerr << "error reading list: expected starting '[' not " << c << std::endl;
                return -1; // error
            }
        }
        else if ( !quotes )
        { // match [ or { or non
            char c = is.get();
            if ( c == '{' || c == '[' )
            {
                foundQuote = c == '{' ? '}' : ']';
            }
            else
            {
                std::cerr << "error reading list: expected default starting '[' or '{' not " << c << std::endl;
                return -1; // error
            }
        }
        TOK expect = ELEM;
        int n = 0;
        while ( is )
        {
            skipSpace( is );
            int x = is.peek();
            if ( ( quotes && quotes[1] ) && quotes[1] == x )
            {
                is.ignore();
                return n;
            }
            if ( !quotes && foundQuote == x )
            {
                is.ignore();
                return n;
            }
            switch ( expect )
            {
            case ELEM:
            {
                skipSpace( is );
                typename Iter::container_type::value_type e;
                is >> e;
                it = std::move( e );
                ++n;
                expect = SEP;
                break;
            }
            case SEP:
            {
                if ( !isspace( sep ) && sep != x )
                    return -2;
                if ( !isspace( sep ) )
                    is.ignore();
                expect = ELEM;
                break;
            }
            default:
            {
                return -4;
            }
            }
        }
        if ( quotes && quotes[1] )
        {
            std::cerr << "error reading list: expected ending " << quotes[1] << std::endl;
            return -3;
        }
        if ( !quotes )
        {
            std::cerr << "error reading list: expected ending " << foundQuote << std::endl;
            return -3;
        }
        return n;
    }
    template<typename Map>
    std::istream &readMap( std::istream &is, Map &amap, const char kvsep = ':', char sep = ',', const char *quotes = "{}" )
    {
        // todo: complete/bugfix
        skipSpace( is );
        if ( quotes[0] != is.peek() )
            return is; // error
        is.ignore();
        while ( is )
        {
            skipSpace( is );
            int x = is.peek();
            if ( quotes[1] == x )
            {
                is.ignore();
                return is;
            }
            else if ( sep == x )
            {
                is.ignore();
            }
            else if ( EOF == x )
            {
                std::cerr << "error reading list: expected ending '}'" << std::endl;
                return is;
            }
            else
            {
                typename Map::key_type key;
                typename Map::mapped_type value;
                is >> key;
                skipSpace( is );
                if ( is.get() != ':' )
                {
                    std::cerr << "error reading map: expected ':'" << std::endl;
                    return is; // error
                }
                skipSpace( is );
                is >> value;
                amap.insert( std::make_pair( key, value ) );
            }
        }
        std::cerr << "error reading map: expected ending '}'" << std::endl;
        return is; // error
    }
    template<typename Key, typename Value>
    std::istream &operator>>( std::istream &os, std::map<Key, Value> &v )
    {
        return readMap( os, v );
    }
    template<typename Key, typename Value>
    std::istream &operator>>( std::istream &os, std::unordered_map<Key, Value> &v )
    {
        return readMap( os, v );
    }
    template<typename Elm>
    std::istream &operator>>( std::istream &os, std::vector<Elm> &v )
    {
        readiterator( os, std::back_inserter( v ) );
        return os;
    }
    template<typename Elm>
    std::istream &operator>>( std::istream &os, std::list<Elm> &v )
    {
        readiterator( os, std::back_inserter( v ) );
        return os;
    }
    template<typename Elm>
    std::istream &operator>>( std::istream &os, std::set<Elm> &v )
    {
        readiterator( os, std::inserter( v ) );
        return os;
    }
} // namespace serialization
} // namespace ftl

#endif /* CONTAINERSERIALIZATION_H_ */
