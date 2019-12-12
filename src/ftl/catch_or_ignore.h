#pragma once
//#ifndef CATCH_OR_IGNORE_H
//#define CATCH_OR_IGNORE_H

//#ifdef UNITTEST_MODE
//#define TEST_MODE
//#include <ftl/catch.hpp>
//#define TEST_FUNC(funcName, ...) TEST_CASE(#funcName __VA_OPT__(, ) __VA_ARGS__)

//#else

//#define TEST_FUNC(funcName, ...) static void funcName()
//#define REQUIRE(expr, ...) assert(expr)

//#endif

//#endif


#include <iostream>
#include <chrono>
#include <sstream>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

#define T_STRINGIFY( x ) #x
#define T_TOSTRING( x ) T_STRINGIFY( x )

#define ASSERT( expr, ... ) assert( ( expr ) && ( "error:" __VA_ARGS__ ) )

#define REQUIRE( expr, ... )                                                                                                                        \
    do                                                                                                                                              \
    {                                                                                                                                               \
        if ( !( expr ) )                                                                                                                            \
        {                                                                                                                                           \
            std::stringstream ss;                                                                                                                   \
            ss << "EvaluationError in " __FILE__ ":" T_TOSTRING( __LINE__ ) ". Expr:\"" #expr << "\", Desc:\"" __VA_ARGS__ << "\"";                 \
            throw UnitTestException( ss.str().c_str() );                                                                                            \
        }                                                                                                                                           \
    } while ( false )

#define REQUIRE_THROW( expr, exception, ... )                                                                                                       \
    do                                                                                                                                              \
    {                                                                                                                                               \
        try                                                                                                                                         \
        {                                                                                                                                           \
            expr;                                                                                                                                   \
        }                                                                                                                                           \
        catch ( const exception &ex )                                                                                                               \
        {                                                                                                                                           \
            break;                                                                                                                                  \
        }                                                                                                                                           \
        std::stringstream ss;                                                                                                                       \
        ss << "NoThrowError in " __FILE__ ":" T_TOSTRING( __LINE__ ) ". Expr:\"" #expr << "\", Desc:\"" __VA_ARGS__ << "\"";                        \
        throw UnitTestException( ss.str().c_str() );                                                                                                \
    } while ( false )


struct UnitTestException : public std::runtime_error
{
    UnitTestException( const char *err ) : std::runtime_error( err )
    {
    }
};

////////////////// static Unitests /////////////////////////////

#define TEST_FUNC( funcName, ... )                                                                                                                  \
    static void funcName();                                                                                                                         \
    static struct funcName##_runner                                                                                                                 \
    {                                                                                                                                               \
        funcName##_runner()                                                                                                                         \
        {                                                                                                                                           \
            std::cout << "++ Start test " << #funcName << ": " __VA_ARGS__ << std::endl;                                                            \
            auto timeStart = std::chrono::steady_clock::now();                                                                                      \
            funcName();                                                                                                                             \
            auto timeDiff = std::chrono::steady_clock::now() - timeStart;                                                                           \
            std::cout << "-- Test ended " << #funcName << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;                    \
        }                                                                                                                                           \
    } s_##funcName##_runner__;                                                                                                                      \
    static void funcName()


////////////////// Managed Unitests /////////////////////////////

#define ADD_TEST_FUNC( funcName, ... )                                                                                                              \
    static void funcName();                                                                                                                         \
    static struct funcName##_register                                                                                                               \
    {                                                                                                                                               \
        funcName##_register()                                                                                                                       \
        {                                                                                                                                           \
            if ( t_get_unittest_registration().tests.count( #funcName ) )                                                                           \
            {                                                                                                                                       \
                throw UnitTestException( "Duplicate Test " #funcName );                                                                             \
            }                                                                                                                                       \
            t_get_unittest_registration().tests[#funcName] = [] {                                                                                   \
                auto &os = *t_get_unittest_registration().os;                                                                                       \
                os << "++ Start test " << #funcName << ": " __VA_ARGS__ << std::endl;                                                               \
                t_get_unittest_registration().currentTest = #funcName;                                                                              \
                auto timeStart = std::chrono::steady_clock::now();                                                                                  \
                try                                                                                                                                 \
                {                                                                                                                                   \
                    funcName();                                                                                                                     \
                    auto timeDiff = std::chrono::steady_clock::now() - timeStart;                                                                   \
                    os << "-- Test ended " << #funcName << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;                   \
                }                                                                                                                                   \
                catch ( const UnitTestException &e )                                                                                                \
                {                                                                                                                                   \
                    auto timeDiff = std::chrono::steady_clock::now() - timeStart;                                                                   \
                    os << "Found test error:" << e.what() << std::endl;                                                                             \
                    os << "** Test error " << #funcName << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;                   \
                    std::string s( #funcName ": " );                                                                                                \
                    s += e.what();                                                                                                                  \
                    throw UnitTestException( s.c_str() );                                                                                           \
                }                                                                                                                                   \
            };                                                                                                                                      \
            t_get_unittest_registration().testNames.push_back( #funcName );                                                                         \
        }                                                                                                                                           \
    } s_##funcName##_register__;                                                                                                                    \
    static void funcName()


using UnitTestFunc = std::function<void()>;

template<class TestFuncT = UnitTestFunc>
struct UnitTestRegistration
{
    std::unordered_map<std::string, TestFuncT> tests;
    std::vector<std::string> testNames;
    std::string currentTest;
    int errCount = 0;
    std::ostream *os = &std::cout;
};

template<class TestFuncT = UnitTestFunc>
UnitTestRegistration<TestFuncT> &t_get_unittest_registration()
{
    static UnitTestRegistration<TestFuncT> s_reg;
    return s_reg;
}

// unit tests main function
template<class TestFuncT = UnitTestFunc>
int unittests_main( int argn, const char *argv[] )
{
    auto usage = []( const std::string err = "" ) -> int {
        if ( !err.empty() )
        {
            std::cout << err << std::endl;
        }
        std::cout << "\n Usage: program [-l||--list] [-h|--help] [-exitonerror] [testnames...] [~excludedtests...] \n"
                     "       ~excludedtests start with ~.\n\n ";
        return !err.empty();
    };
    auto printTestNames = [] {
        const auto &names = t_get_unittest_registration<TestFuncT>().testNames;
        for ( const auto &name : names )
        {
            std::cout << name << std::endl;
        }
        std::cout << " Totally " << names.size() << " tests." << std::endl;
    };
    bool a_exitOnError = false;
    auto &regs = t_get_unittest_registration<TestFuncT>();

    std::vector<std::string> whitelist;
    std::unordered_set<std::string> blacklist; // excluded test names start with '~'
    for ( int i = 1; i < argn; ++i )
    {
        std::string a = argv[i];
        if ( a == "-l" || a == "--list" )
        {
            printTestNames();
            return 0;
        }
        if ( a == "-h" || a == "--help" )
            return usage();

        if ( a == "-exitonerror" )
            a_exitOnError = true;
        else if ( argv[i][0] == '~' )
            blacklist.insert( &argv[i][1] );
        else
            whitelist.push_back( argv[i] );
    }

    const auto &testNames = whitelist.empty() ? regs.testNames : whitelist;
    std::string errors;
    for ( const auto &name : testNames )
    {
        try
        {
            if ( !blacklist.count( name ) )
                regs.tests[name]();
        }
        catch ( const UnitTestException &e )
        {
            errors += e.what();
            errors += "\n";
            ++regs.errCount;
            if ( a_exitOnError )
                break;
        }
    }
    if ( regs.errCount == 0 )
    {
        *regs.os << "Succedded running all " << testNames.size() << " test cases." << std::endl;
    }
    else
    {
        *regs.os << "All tests errors:\n" << errors << std::endl;
        *regs.os << regs.errCount << " errors in " << testNames.size() << " test cases." << std::endl;
    }
    return regs.errCount;
}
