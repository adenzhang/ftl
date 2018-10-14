#ifndef CATCH_OR_IGNORE_H
#define CATCH_OR_IGNORE_H

#ifdef UNITTEST_MODE
    #define TEST_MODE
    #include <ftl/catch.hpp>
    #define TEST_FUNC(funcName) TEST_CASE(#funcName)

#else

    #define TEST_FUNC(funcName) static void funcName()
    #define REQUIRE( expr) assert(expr)

#endif

#endif