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
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <iostream>

namespace ftl
{
class TestCase;
struct CatchMain
{
    virtual void AddTestCase( TestCase *test ) = 0;

    virtual bool Run() = 0;
};
CatchMain &GetCatchMain();
struct RequireException : public std::runtime_error
{
    RequireException( const char *err ) : std::runtime_error( err )
    {
    }
};
struct RerunException
{
};

struct TestCase
{
    TestCase( const std::string &name ) : testCaseName( name )
    {
        GetCatchMain().AddTestCase( this );
    }
    virtual bool Run()
    {
        std::cout << "-- Running test case: " << testCaseName << std::endl;
    RERUN:
        nRequires = 0;
        try
        {
            RunImpl();
        }
        catch ( const RerunException & )
        {
            goto RERUN;
        }
        catch ( const RequireException &err )
        {
            std::cerr << "test case:" << testCaseName << ": ERROR:" << err.what() << std::endl;
            return false;
        }
        return true;
    }

    bool AddSection( const std::string &name )
    {
        return sectionNames.insert( name ).second;
    }

    size_t nRequires = 0;

protected:
    virtual void RunImpl() = 0;
    std::string testCaseName;
    std::unordered_set<std::string> sectionNames;
};


struct CatchMainImpl : public CatchMain
{
    std::vector<TestCase *> test_cases;

    void AddTestCase( TestCase *test ) override
    {
        test_cases.push_back( test );
    }

    bool Run() override
    {
        bool ok = true;
        size_t nRequires = 0;
        for ( auto &test : test_cases )
        {
            if ( !test->Run() )
                ok = false;
            nRequires += test->nRequires;
        }
        return ok;
    }
};
} // namespace ftl

#define TEST_CASE_IMPL( dest, id )                                                                                                                  \
    static struct TEST_##id : public TestCase                                                                                                       \
    {                                                                                                                                               \
        void RunImpl() override;                                                                                                                    \
    } s_test_##id;                                                                                                                                  \
    void TEST_##id ::RunImpl()                                                                                                                      \
    {                                                                                                                                               \
    }

#define TEST_CASE ( name ) TEST_CASE_IMPL( name, __LINE__ )

#define SECTION( secName ) for ( bool ok = this->AddSection( secName ); ok; throw RerunException() )



#define CATCH_CONFIG_MAIN                                                                                                                           \
    ftl::CatchMain &GetCatchMain()                                                                                                                  \
    {                                                                                                                                               \
        static ftl::CatchMainImpl s_catchMain;                                                                                                      \
        return s_catchMain;                                                                                                                         \
    }                                                                                                                                               \
    int main( int argn, const char *args )                                                                                                          \
    {                                                                                                                                               \
        GetCatchMain().Run();                                                                                                                       \
    }

#define REQUIRE( ... )                                                                                                                              \
    if ( ++nRequires; !( __VA_ARGS ) )                                                                                                              \
        throw RequireException(##__VA_ARGS );
