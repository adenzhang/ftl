#include <ftl/unittest.h>
#include <ftl/Utf8.h>

using namespace std;
using namespace jz;
ADD_TEST_CASE( utf8_tests )
{
    char buf[5];
    char s[] = "a我1";

    vector<int> unicodeValues;
    copy( Utf8Int::Iterator( s ), Utf8Int::Iterator(), back_inserter( unicodeValues ) );

    cout << "- utf8 decoding" << endl;
    for ( auto c : Utf8Int( s ) )
    {
        unicodeValues.push_back( c );
        cout << c << ",";
    }
    cout << "\n- utf8 encoding" << endl;
    for ( auto c : unicodeValues )
    {
        cout << ( Utf8Int::encodeUtf8( buf, c ) ? buf : "invalideCode" ) << ",";
    }
}
