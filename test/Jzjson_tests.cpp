#include <ftl/unittest.h>
#include <ftl/Jzjson.h>

ADD_TEST_CASE( Jzjson_tests )
{
    std::stringstream ss( R"(
[
   { addOrder: { qty: 12, side: "buy it" }}, // comment
   { action: addOrder,  qty: 14, price: 12.9, side: sell, others:{ "first name" : jack, score: 100.5, languages: [ 2, 3, 4] } },

   { cancelOrder: { qty: 23, price: -25, side: sell , comment: "" }},
   [ 1, 3, {a : 2, b : '234 3' }, "asd2 32"]

                          {}
                          []
]
)" );

    jz::JsonNode node;
    REQUIRE( jz::jsonSerializer.read( node, ss, std::cerr ) );
    std::cout << "Parsed:" << node << std::endl;

    REQUIRE_EQ( node[0]["addOrder"]["side"].str(), "buy it" );
    REQUIRE_EQ( node[1]["action"].str(), "addOrder" );
    REQUIRE_EQ( node[2]["cancelOrder"]["qty"].toInt(), 23 );
    REQUIRE_EQ( node[2]["cancelOrder"]["comment"].str(), "" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["side"].str(), "sell" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["price"].toInt(), -25 );
    REQUIRE_EQ( node.childWithKeyValue( "action", "addOrder" )["qty"].toInt(), 14 );
    REQUIRE_EQ( node[4].size(), 0u );
    REQUIRE_EQ( node[5].size(), 0u );
}

ADD_TEST_CASE( Jzon_tests )
{
    jz::JsonNode node;

    SECTION( "basic" )
    {
        std::stringstream ss( R"(

                { addOrder { qty 12
                             side "buy it",
                             price : +23.3
                             comment "this is
                                  muliple line"}
                  addOrder { qty 23
                            price: -12
                         }   // dup keys are combined into vec addOrder [ {qty 23...}, { qty 23...} ]

                  samples [223 321 [34, 3] 2]
                  dates []
                  owners {}
                } // comment
            )" );

        REQUIRE( jz::jzonSerializer.read( node, ss, std::cerr ) );
        std::cout << "Parsed:" << node << std::endl;
        //        jz::jzonSerializer.printJsonCompact( std::cout, node ) << std::endl;

        REQUIRE_EQ( node["addOrder"][0]["side"].str(), "buy it" );
        REQUIRE_EQ( node["dates"].size(), 0u );
        REQUIRE_EQ( node["owners"].size(), 0u );
    }

    SECTION( "omit vec brackets" )
    {
        std::stringstream ss( R"(
                    {
                      samples [23e3 [321 34] 234]
                    } // comment
                )" );

        REQUIRE( jz::jzonSerializer.read( node, ss, std::cerr ) );
        std::cout << "Parsed:" << node << std::endl;
        REQUIRE_EQ( node["samples"][0].str(), "23e3" );
    }
}
