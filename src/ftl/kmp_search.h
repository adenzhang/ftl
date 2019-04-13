#pragma once

#include <tuple>
#include <vector>

namespace ftl
{

class KMP
{
public:
    using this_type = KMP;
    using position = size_t; // index pattern

    ///////////////////////////////////////////////////////////////////
    //// Static helper functions
    ///////////////////////////////////////////////////////////////////

    // populate longest prefix suffix array
    // lps[i] = longest proper prefix of pat[0..i] which is also a suffix of pat[0..i]
    template<class RandomIter, class VectorIter>
    static void make_prefix_suffix0( RandomIter it, RandomIter itEnd, VectorIter itPrefix )
    {
        *itPrefix = 0;
        for ( RandomIter i = it + 1, iLast = it; i != itEnd; ) // iLast: prefix end or len
        {
            if ( *i == *iLast )
            {
                ++iLast;
                *( itPrefix + std::distance( it, i ) ) = std::distance( it, iLast ); // lps[i] = len
                ++i;
            }
            else
            {
                if ( iLast == it ) // prefix len == 0
                {
                    *( itPrefix + std::distance( it, i ) ) = 0; // lps[i] = 0
                    ++i;
                }
                else
                    iLast = it + *( itPrefix + std::distance( it, iLast ) - 1 ); // len = lps[len-1]
            }
        }
    }
    template<class RandomIter, class VectorIter>
    static void make_prefix_suffix( RandomIter it, RandomIter itEnd, VectorIter itPrefix )
    {
        *itPrefix = 0;
        for ( RandomIter i = it + 1; i != itEnd; ++i ) // iLast: prefix end or len
        {
            auto j = *( itPrefix + std::distance( it, std::prev( i ) ) ); // j = LPS[i-1]
            while ( j > 0 && *i != *( it + j ) )
                j = *( itPrefix + j - 1 ); // j = LPS[j-1]
            if ( *i == *( it + j ) ) // P[i] == P[j]
                ++j;
            *( itPrefix + std::distance( it, i ) ) = j; // LPS[i] = j
        }
    }
    template<class SourceIter, class PatternIter, class LPSVec>
    static SourceIter search( SourceIter &itBegin, SourceIter itEnd, PatternIter patBegin, PatternIter patEnd, const LPSVec &LPS, position &patPos )
    {
        const size_t M = std::distance( patBegin, patEnd );
        auto &i = itBegin;
        auto &j = patPos; // pat pos
        while ( i != itEnd )
        {
            if ( *( patBegin + j ) == *i )
            {
                ++j;
                ++i;
            }
            if ( j == M ) // matches, return the matched position
            {
                auto res = i - j;
                j = *( LPS + j - 1 );
                return res;
            }
            else if ( i != itEnd && *( patBegin + j ) != *i ) // mimatch after j matches
            {
                if ( j == 0 )
                    ++i;
                else
                    j = *( LPS + j - 1 );
            }
        }
        return itEnd;
    }

    template<class SourceIter, class PatternIter, class VectorIter>
    static std::tuple<SourceIter, SourceIter, PatternIter, PatternIter, VectorIter, position> init_search_pat(
            SourceIter itBegin, SourceIter itEnd, PatternIter patBegin, PatternIter patEnd, VectorIter LPS )
    {
        make_prefix_suffix( patBegin, patBegin, LPS );
        return {itBegin, itEnd, patBegin, patEnd, LPS, position( 0 )};
    }

    template<class SourceIter, class... Args>
    static SourceIter search_next_pat( std::tuple<SourceIter, Args...> &p )
    {
        return search( std::get<0>( p ), std::get<1>( p ), std::get<2>( p ), std::get<3>( p ), std::get<4>( p ), std::get<5>( p ) );
    }

    ///////////////////////////////////////////////////////////////////
    //// Object member functions
    ///////////////////////////////////////////////////////////////////

    template<class RandomIter>
    this_type &init_pattern( RandomIter it, RandomIter itEnd )
    {
        LPS.resize( std::distance( it, itEnd ) );
        make_prefix_suffix( it, itEnd, LPS.begin() );
        return *this;
    }

    template<class SourceIter, class PatternIter>
    std::tuple<SourceIter, SourceIter, PatternIter, PatternIter, position> init_search( SourceIter itBegin,
                                                                                        SourceIter itEnd,
                                                                                        PatternIter patBegin,
                                                                                        PatternIter patEnd )
    {
        init_pattern( patBegin, patEnd );
        return {itBegin, itEnd, patBegin, patEnd, position( 0 )};
    }

    template<class SourceIter, class... Args>
    SourceIter search_next( std::tuple<SourceIter, Args...> &p ) const
    {
        return search( std::get<0>( p ), std::get<1>( p ), std::get<2>( p ), std::get<3>( p ), LPS.begin(), std::get<4>( p ) );
    }

protected:
    std::vector<size_t> LPS;
};
/**
    {
        KMP kmp;
        string txt = "AAAAABAAAABA", pat = "AAAA";
        auto state = kmp.init_search( txt.begin(), txt.end(), pat.begin(), pat.end() );
        REQUIRE( distance( txt.begin(), kmp.search_next( state ) ) == 0 );
        REQUIRE( distance( txt.begin(), kmp.search_next( state ) ) == 1 );
        REQUIRE( distance( txt.begin(), kmp.search_next( state ) ) == 6 );
        REQUIRE( kmp.search_next( state ) == txt.end() );
    }
 */
} // namespace ftl
