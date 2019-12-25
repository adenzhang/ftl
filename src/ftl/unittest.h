#pragma once

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


#define REQUIRE_OP( OP, X, Y, ... )                                                                                                                 \
    do                                                                                                                                              \
    {                                                                                                                                               \
        auto rx = ( X );                                                                                                                            \
        auto ry = ( Y );                                                                                                                            \
        if ( !( rx OP ry ) )                                                                                                                        \
        {                                                                                                                                           \
            std::stringstream ss;                                                                                                                   \
            ss << "EQEvaluationError in " __FILE__ ":" T_TOSTRING( __LINE__ ) ". Expected:\"" #X " " #OP " " #Y << "\", got " << rx << " " #OP " "  \
               << ry << ". Desc:\"" __VA_ARGS__ << "\"";                                                                                            \
            throw UnitTestException( ss.str().c_str() );                                                                                            \
        }                                                                                                                                           \
    } while ( false )

#define REQUIRE_EQ( X, Y, ... ) REQUIRE_OP( ==, X, Y, __VA_ARGS__ )
#define REQUIRE_NE( X, Y, ... ) REQUIRE_OP( !=, X, Y, __VA_ARGS__ )

#define REQUIRE( expr, ... )                                                                                                                        \
    do                                                                                                                                              \
    {                                                                                                                                               \
        if ( !( expr ) )                                                                                                                            \
        {                                                                                                                                           \
            std::stringstream ss;                                                                                                                   \
            ss << "EvaluationError in " __FILE__ ":" T_TOSTRING( __LINE__ ) ". Expr:\"" #expr << "\", Desc:\"" __VA_ARGS__ << "\"";                 \
            throw UnitTestRequireException( ss.str().c_str() );                                                                                     \
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
        throw UnitTestRequireException( ss.str().c_str() );                                                                                         \
    } while ( false )

#define SECTION( name, ... ) for ( auto ok = AddSectionName( name ); ok; throw UnitTestRerunException( name ) )

struct UnitTestRequireException : public std::runtime_error
{
    UnitTestRequireException( const char *err ) : std::runtime_error( err )
    {
    }
};

struct UnitTestRerunException : public std::runtime_error
{
    UnitTestRerunException( const char *err = "" ) : std::runtime_error( err )
    {
    }
};

////////////////// static Unitests /////////////////////////////

#define TEST_FUNC( funcName, ... )                                                                                                                  \
    static struct funcName##_runner                                                                                                                 \
    {                                                                                                                                               \
        void funcName();                                                                                                                            \
        funcName##_runner()                                                                                                                         \
        {                                                                                                                                           \
            std::cout << "++ Start test " << #funcName << ": " __VA_ARGS__ << std::endl;                                                            \
            auto timeStart = std::chrono::steady_clock::now();                                                                                      \
            for ( bool rerun = true; rerun; )                                                                                                       \
            {                                                                                                                                       \
                try                                                                                                                                 \
                {                                                                                                                                   \
                    funcName();                                                                                                                     \
                    rerun = false;                                                                                                                  \
                }                                                                                                                                   \
                catch ( const UnitTestRerunException & )                                                                                            \
                {                                                                                                                                   \
                    os << "  Section completed: " << m_name << "/" << e.what() << "\n";                                                             \
                }                                                                                                                                   \
            }                                                                                                                                       \
            auto timeDiff = std::chrono::steady_clock::now() - timeStart;                                                                           \
            std::cout << "-- Test ended " << #funcName << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;                    \
        }                                                                                                                                           \
        bool AddSectionName( const std::string &section )                                                                                           \
        {                                                                                                                                           \
            return m_executedSections.insert( section ).second;                                                                                     \
        }                                                                                                                                           \
        std::unordrered_set<std::string> m_executedSections;                                                                                        \
    } s_##funcName##_runner__;                                                                                                                      \
    void funcName##_runner::funcName()


////////////////// Managed Unitests /////////////////////////////

struct UnitTestCaseBase;
struct UnitTestRegistration
{
    std::unordered_map<std::string, UnitTestCaseBase *> tests;
    std::vector<std::string> testNames;
    std::string currentTest;
    int errCount = 0;
    std::ostream *os = &std::cout;
};

template<class UnitTestRegistrationT = UnitTestRegistration>
UnitTestRegistrationT &t_get_unittest_registration()
{
    static UnitTestRegistrationT s_reg;
    return s_reg;
}
struct UnitTestCaseBase
{
    virtual void RunTestImpl() = 0;
    virtual ~UnitTestCaseBase() = default;

    UnitTestCaseBase( const std::string name ) : m_name( name )
    {
        if ( t_get_unittest_registration().tests.count( name ) )
        {
            throw UnitTestRequireException( ( name + " Duplicate name" ).c_str() );
        }
        t_get_unittest_registration().tests[name] = this;
        t_get_unittest_registration().testNames.push_back( name );
    }
    bool AddSectionName( const std::string &section )
    {
        return m_executedSections.insert( section ).second;
    }
    void RunTest()
    {
        auto &os = *t_get_unittest_registration().os;
        os << "++ Start test " << m_name << std::endl;
        t_get_unittest_registration().currentTest = m_name;
        auto timeStart = std::chrono::steady_clock::now();
        for ( bool rerun = true; rerun; )
        {
            try
            {
                this->RunTestImpl();
                rerun = false;
            }
            catch ( const UnitTestRerunException &e )
            {
                os << "  Section completed: " << m_name << "/" << e.what() << "\n";
            }
            catch ( const UnitTestRequireException &e )
            {
                auto timeDiff = std::chrono::steady_clock::now() - timeStart;
                os << "Found test error:" << e.what() << std::endl;
                os << "** Test error " << m_name << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;
                std::string s( m_name + ": " );
                s += e.what();
                throw UnitTestRequireException( s.c_str() );
            }
        }
        auto timeDiff = std::chrono::steady_clock::now() - timeStart;
        os << "-- Test ended " << m_name << ", time elapsed (ns): " << timeDiff.count() << std::endl << std::endl;
    }

    const std::string &GetName() const
    {
        return m_name;
    }
    std::unordered_set<std::string> m_executedSections;
    std::string m_name;
};

#define ADD_TEST_FUNC( funcName, ... )                                                                                                              \
    static struct funcName##_TestCase : public UnitTestCaseBase                                                                                     \
    {                                                                                                                                               \
        void RunTestImpl() override;                                                                                                                \
        funcName##_TestCase() : UnitTestCaseBase( #funcName )                                                                                       \
        {                                                                                                                                           \
        }                                                                                                                                           \
    } s_##funcName##_TestCase__;                                                                                                                    \
    void funcName##_TestCase::RunTestImpl()


// unit tests main function
template<class TestCase = UnitTestCaseBase>
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
        const auto &names = t_get_unittest_registration().testNames;
        for ( const auto &name : names )
        {
            std::cout << name << std::endl;
        }
        std::cout << " Totally " << names.size() << " tests." << std::endl;
    };
    bool a_exitOnError = false;
    auto &regs = t_get_unittest_registration();

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
    const auto start = std::chrono::steady_clock::now();
    for ( const auto &name : testNames )
    {
        try
        {
            if ( !blacklist.count( name ) )
                regs.tests[name]->RunTest();
        }
        catch ( const UnitTestRequireException &e )
        {
            errors += e.what();
            errors += "\n";
            ++regs.errCount;
            if ( a_exitOnError )
                break;
        }
    }
    const auto stop = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>( stop - start );
    if ( regs.errCount == 0 )
    {
        *regs.os << "Succedded running all " << testNames.size() << " test cases. Tests took " << duration.count() << " us." << std::endl;
    }
    else
    {
        *regs.os << "All tests errors:\n" << errors << std::endl;
        *regs.os << regs.errCount << " errors in " << testNames.size() << " test cases. Tests took " << duration.count() << " us." << std::endl;
    }
    return regs.errCount;
}

#define UNITTEST_MAIN                                                                                                                               \
    int main( int argc, const char *argv[] )                                                                                                        \
    {                                                                                                                                               \
        return unittests_main( argc, argv );                                                                                                        \
    }
