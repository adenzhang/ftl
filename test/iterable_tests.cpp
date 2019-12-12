#include "ftl/iterable.h"
#include <ftl/catch_or_ignore.h>
#include <iostream>
#include <list>
#include <map>

using namespace ftl;
ADD_TEST_FUNC( iterable_tests )
{
    using namespace func_operator;

    std::list<int> il{1, 2, 3};
    std::vector<int> iv{1, 2, 3};
    std::map<int, int> im{{2, 3}, {3, 4}};

    const int arr[] = {
            23,
            234,
    };

    for_each_iterable( make_iterable( il, il.begin(), il.end() ), []( auto &e ) { std::cout << " el: " << e; } );

    make_iterable( iv ).for_each( []( auto &e ) {
        e += 3;
        std::cout << " el: " << e;
    } );

    auto sum = make_iterable( iv ).reduce( std::plus<int>(), 0 );

    std::cout << " sum: " << sum << std::endl;
    il % make_iterable % bind_2nd( for_each_iterable, []( auto &e ) { std::cout << " il: " << e; } );

    static_cast<const std::vector<int> &>( iv ) % make_iterable % bind_2nd( for_each_iterable, []( auto &e ) {
        //        e += 3; unable to change const value
        std::cout << " iv: " << e;
    } );

    static_cast<const std::map<int, int> &>( im ) % make_iterable %
            bind_2nd( for_each_iterable, []( auto &e ) { std::cout << " im: {" << e.first << "->" << e.second << "}"; } );

    arr % make_iterable % bind_2nd( for_each_iterable, []( auto &e ) { std::cout << " arr: " << e; } );

    std::list<std::string>{"abc", "234", "23ws"} % make_iterable % bind_2nd( for_each_iterable, []( auto &e ) { std::cout << " arr: " << e; } );

    3 % []( auto e ) {
        e += 10;
        std::cout << " el: " << e << std::endl;
        return e;
    } % []( auto e ) {
        e += 100;
        std::cout << " el: " << e << std::endl;
        return e;
    };

    void_var %
            []() {
                std::cout << " starting " << std::endl;
                return 4;
            } %
            make_const_func( 4 ) %
            []( int y ) {
                int x = y + 10;
                std::cout << " processing " << x << std::endl;
            } %
            []( void ) { std::cout << " ending " << std::endl; };
}
