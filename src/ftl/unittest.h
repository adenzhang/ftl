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

#define EXIT_OR_THROW( ERRORSTR, EXCEPTIONTYPE )                                                                                                    \
    if ( DoesAbortOnError() )                                                                                                                       \
    {                                                                                                                                               \
        GetOStream() << ERRORSTR << std::endl << std::flush;                                                                                        \
        abort();                                                                                                                                    \
    }                                                                                                                                               \
    else                                                                                                                                            \
        throw EXCEPTIONTYPE( ERRORSTR.c_str() );

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
            EXIT_OR_THROW( ss.str(), UnitTestRequireException );                                                                                    \
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
            EXIT_OR_THROW( ss.str(), UnitTestRequireException );                                                                                    \
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
        EXIT_OR_THROW( ss.str(), UnitTestRequireException );                                                                                        \
    } while ( false )

#define SECTION( name, ... ) for ( auto ok = StartSection( name ); ok; throw UnitTestRerunException( name ) )

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

////////////////// Managed Unitests /////////////////////////////

struct UnitTestCaseBase;
struct UnitTestRegistration
{
    std::unordered_map<std::string, UnitTestCaseBase *> tests;
    std::vector<std::string> testNames;
    std::string currentTest;
    int errCount = 0;
    std::ostream *os = &std::cout;
    bool abortOnError = false;
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
    bool StartSection( const std::string &section )
    {
        if ( m_executedSections.insert( section ).second )
        {
            m_currSection = section;
            return true;
        }
        return false;
    }
    std::string GetCurrentSection() const
    {
        return m_currSection;
    }
    void RunTest()
    {
        auto &os = GetOStream();
        os << "++ Start test " << m_name << std::endl;
        t_get_unittest_registration().currentTest = m_name;
        auto timeStart = std::chrono::steady_clock::now();
        for ( bool rerun = true; rerun; )
        {
            try
            {
                m_currSection.clear();
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
    std::ostream &GetOStream()
    {
        return *t_get_unittest_registration().os;
    }

    bool DoesAbortOnError() const
    {
        return t_get_unittest_registration().abortOnError;
    }
    std::unordered_set<std::string> m_executedSections;
    std::string m_name;
    std::string m_currSection;
    int m_requirements = 0;
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
        std::cout << "\n Usage: program [-l||--list] [-h|--help] [-onerror abort|return|continue] [testnames...] [~excludedtests...] \n"
                     "    ~excludedtests start with ~.\n"
                     "    -onerror abort: abort program; return, skip rest test cases; default continue to run other testcases.\n\n";
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

        if ( a == "-onerror" )
        {
            if ( i == argn - 1 )
                return usage( "No argument for -onerror" );
            a = argv[++i];
            if ( a == "abort" )
                t_get_unittest_registration().abortOnError = true;
            else if ( a == "return" )
                a_exitOnError = true;
            else if ( a == "continue" )
                a_exitOnError = false;
            else
                return usage( "Wrong -onerror argument: " + a );
        }
        else if ( argv[i][0] == '~' )
            blacklist.insert( &argv[i][1] );
        else
        {
            if ( !regs.tests.count( a ) )
                return usage( "No test case found: " + a );
            whitelist.push_back( argv[i] );
        }
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
