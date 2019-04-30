#pragma once
#include <type_traits>
#include <stdexcept>
#include <optional>

// class Generator
//      - OptionalOrPointer next()
//      - iterator begin()
//      - iterator end()
//
// Constructed by functor Lambda. Lambda::oprator() returns optional<T> or T*
template<class Lambda, class T = std::remove_reference_t<decltype( *std::declval<Lambda>()() )>>
class Generator
{
public:
    using OptionalOrPointer = std::remove_reference_t<decltype( std::declval<Lambda>()() )>;
    struct iterator
    {
        Generator *cont = nullptr;
        OptionalOrPointer data;

        iterator( Generator *c = nullptr, OptionalOrPointer d = OptionalOrPointer{} ) : cont( c ), data( d )
        {
        }

        bool operator==( const iterator &a ) const
        {
            return is_end() && a.is_end();
        }
        bool operator!=( const iterator &a ) const
        {
            return !this->operator==( a );
        }

        OptionalOrPointer operator->() const
        {
            return data;
        }
        auto &operator*() const
        {
            return *data;
        }

        iterator &operator++()
        {
            data = cont->next();
            return *this;
        }
        iterator operator++( int )
        {
            iterator tmp{cont, std::move( data )};
            data = cont->next();
            return std::move( tmp );
        }
        bool is_end() const
        {
            return !data;
        }
    };


    Generator( Lambda &&lambda ) : lambda( std::forward<Lambda>( lambda ) )
    {
    }

    Generator( const Generator &a ) = default;
    Generator( Generator &&a ) = default;
    Generator &operator=( const Generator &a ) = default;
    Generator &operator=( Generator &&a ) = default;

    iterator begin()
    {
        return iterator{this, lambda()};
    }
    iterator end()
    {
        return iterator{this};
    }

    OptionalOrPointer next()
    {
        return lambda();
    }

protected:
    Lambda lambda;
};


template<class Iter>
auto make_generator_iter( Iter itBegin, Iter itEnd )
{
    using ValueT = std::remove_reference_t<decltype( *std::declval<Iter>() )>;
    return Generator( [=]() mutable -> ValueT * { return itBegin == itEnd ? nullptr : &( *( itBegin++ ) ); } );
}

template<class Iter>
auto make_generator_iter_inf( Iter itBegin )
{
    using ValueT = std::remove_reference_t<decltype( *std::declval<Iter>() )>;
    return Generator( [=]() mutable -> ValueT * { return &( *( itBegin++ ) ); } );
}


// if inc == 0, auto calc inc = 1 if start <= end, inc = -1 if  start > end
template<class Int>
auto make_generator_range( Int start, Int end, std::enable_if_t<std::is_integral_v<Int>, std::make_signed_t<Int>> inc = 0 )
{
    using ValueT = Int;
    if ( inc == 0 )
    {
        inc = end < start ? -1 : 1;
    }
    return Generator( [=]() mutable -> std::optional<ValueT> {
        if ( inc > 0 && start >= end || inc < 0 && start <= end )
            return {};
        else
        {
            std::optional<ValueT> res{start};
            start += inc;
            return std::move( res );
        }
    } );
}

// inc can be 0.
template<class Int, std::enable_if_t<std::is_integral_v<Int>>>
auto make_generator_range_inf( Int start, int inc = 1 )
{
    using ValueT = Int;
    return Generator( [=]() mutable -> std::optional<ValueT> { return std::optional<ValueT>( ( start++ ) + inc ); } );
}

/***  usage:
static int test_generator()
{
    const std::vector<int> A = {1, 2};
    auto gen = make_generator_iter( A.begin(), A.end() );

    for ( const auto &e : gen )
    {
        std::cout << e << ", ";
    }

    auto gen2 = make_generator_range( 10, 20, 3 );

    for ( const auto &e : gen2 )
    {
        std::cout << e << ", ";
    }
    return 0;
}

*/
