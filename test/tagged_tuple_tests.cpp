#include <ftl/ftl.h>
#include <ftl/tagged_tuple.h>
#include <iostream>
#include <string>

using namespace ftl;

ADD_TEST_FUNC( tagged_tuple_tests )
{

    {
        std::cout << "false:" << can_construct_dataframe<std::string, int>() << std::endl;
        std::cout << "false:" << can_construct_dataframe<std::string, std::wstring>() << std::endl;
        std::cout << "true:" << can_construct_dataframe<std::vector<int>, std::vector<std::string>>() << std::endl;
        std::cout << "true:" << can_construct_dataframe<std::vector<int>, std::deque<std::string>>() << std::endl;

        TaggedTuple<Tagged<"name"_tref, std::string>, TaggedType<decltype( "id"_t ), int>, Tagged<"score"_tref, double>> taggedTup;
        TaggedTuple<Tagged<"age"_tref, int>, Tagged<"address"_tref, std::string>> info{10, "New York"};
        taggedTup["name"_t] = "asdf";
        taggedTup["id"_t] = 23;

        auto subtup = taggedTup.clone( "name"_t, "id"_t );
        subtup["name"_t] += "+23";

        auto &constup = taggedTup;
        auto subreftup = constup.ref( "name"_t, "id"_t );
        subreftup["name"_t] = "abc";

        std::cout << "name:" << taggedTup["name"_t] << ",  subtuple same name:" << subtup["name"_t] << subtup.ref_tagged_value_at<0>() << std::endl;

        auto subsubref = subreftup.ref( "name"_t, "id"_t );
        subsubref["name"_t] = "subsubref";

        auto refinfo = info.ref();
        auto combined = subreftup + refinfo; // concat TaggedTuples.

        info["address"_t] = "New Jersey";

        std::cout << "name:" << taggedTup["name"_t] << ",  subtuple same name:" << subtup["name"_t] << std::endl;

        std::cout << refinfo << std::endl;

        std::cout << "Combined: " << combined << std::endl;
    }


    { // tagged_tuple_dataframe_tests
        DataFrame<Tagged<"name"_tref, std::string>, Tagged<"id"_tref, int>, Tagged<"score"_tref, double>> df;

        df.reserve( 10 );

        df.append_row( std::string( "Jason" ), 2343, 2.3 );
        df.append_row( "Tom A", 123, 849 );

        std::cout << "df :" << df << std::endl;

        df.rowref( 1 )["name"_t] = "Tom B";
        df["name"_t][0] = "Jason B";

        std::cout << "df: " << df << std::endl;

        auto subref = df.ref( "name"_t, "id"_t );

        subref["id"_t][1] = 111;

        std::cout << "ref df: " << subref << std::endl;

        std::cout << "df id changed: " << df << std::endl;


        DataFrame<Tagged<"age"_tref, int>, Tagged<"address"_tref, std::string>> df1( std::vector<int>{10, 20},
                                                                                     std::vector<std::string>{"address NY", "address NJ"} );

        auto df1ref = df1.ref();

        auto df2 = df + df1ref; // df2 references to df1, copy of df.

        df1["address"_t][1] = "address CT";

        std::cout << "df1ref :" << df1ref << std::endl;
        std::cout << "combined df2 address[1] is changed to CT :" << df2 << std::endl;

        df2.append_row( "Sam C", 234, 10, 100, "address CC" );

        REQUIRE( df2.aligned_size() == 3 );
        std::cout << "combined df2 3 rows:" << df2 << std::endl;

        REQUIRE( df1.aligned_size() == 3 );
        std::cout << "original df1 3 rows:" << df1 << std::endl;

        REQUIRE( df.aligned_size() == 2 );
        std::cout << "df1 is still 2 row: no change:" << df << std::endl;


        { //// test stack
            DataFrame<Tagged<"name"_tref, std::string>, Tagged<"id"_tref, int>> df( std::vector<std::string>{"A", "B"}, std::vector<int>{1, 2} );
            df.stack( TaggedTuple<Tagged<"name"_tref, std::string>, Tagged<"age"_tref, int>, Tagged<"id"_tref, int>>{std::string( "C" ), 23, 3} );

            REQUIRE( df.size() == 3 );
            std::cout << "stacked dataframe 3 rows:" << df << std::endl;

            DataFrame<Tagged<"name"_tref, std::string>, Tagged<"age"_tref, int>, Tagged<"id"_tref, int>> df1(
                    std::vector<std::string>{"D", "E"}, std::vector<int>{20, 50}, std::vector<int>{5, 2} );

            df.stack( df1 );

            REQUIRE( df.size() == 5 );
            std::cout << "stacked dataframe 5 rows:" << df << std::endl;

            auto stacked_copy = df.stack( df1 );

            REQUIRE( stacked_copy.size() == 7 );
            std::cout << "stacked copy dataframe 7 rows:" << stacked_copy << std::endl;
        }
    }
}
