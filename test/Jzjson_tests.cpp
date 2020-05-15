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
]
)" );

    jz::JsonNode node;
    REQUIRE( jz::jsonSerilizer.read( node, ss, std::cerr ) );
    std::cout << "Parsed:" << node << std::endl;

    REQUIRE_EQ( node[0]["addOrder"]["side"].str(), "buy it" );
    REQUIRE_EQ( node[1]["action"].str(), "addOrder" );
    REQUIRE_EQ( node[2]["cancelOrder"]["qty"].toInt(), 23 );
    REQUIRE_EQ( node[2]["cancelOrder"]["comment"].str(), "" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["side"].str(), "sell" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["price"].toInt(), -25 );
    REQUIRE_EQ( node.childWithKeyValue( "action", "addOrder" )["qty"].toInt(), 14 );
}
