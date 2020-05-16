#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>

namespace jz
{
/********************************************************
 Header only Json format.
 Examples:

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
    REQUIRE( jz::JsonSerializer.read( node, ss, std::cerr ) );
    std::cout << "Parsed:" << node << std::endl;

    REQUIRE_EQ( node[0]["addOrder"]["side"].str(), "buy it" );
    REQUIRE_EQ( node[1]["action"].str(), "addOrder" );
    REQUIRE_EQ( node[2]["cancelOrder"]["qty"].toInt(), 23 );
    REQUIRE_EQ( node[2]["cancelOrder"]["comment"].str(), "" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["side"].str(), "sell" );
    REQUIRE_EQ( node.childWithKey( "cancelOrder" )["price"].toInt(), -25 );
    REQUIRE_EQ( node.childWithKeyValue( "action", "addOrder" )["qty"].toInt(), 14 );
}

********************************************************/

/// @brief DynNode which can be of string type, map type, vector type.
template<class StrT = std::string>
class DynNode
{
public:
    enum NodeType
    {
        STR_NODE_TYPE,
        MAP_NODE_TYPE,
        VEC_NODE_TYPE,
        NIL_NODE_TYPE, // not used as valid node type
    };

    using this_type = DynNode<StrT>;
    using StrType = StrT;
    using DynNodePtr = std::unique_ptr<this_type>;
    using MapType = std::unordered_map<StrType, DynNodePtr>;
    using VecType = std::vector<DynNodePtr>;

    static const size_t STR_SIZE = sizeof( StrType );
    static const size_t MAP_SIZE = sizeof( MapType );
    static const size_t VEC_SIZE = sizeof( VecType );

    DynNode( const StrType &s )
    {
        nodeType = STR_NODE_TYPE;
        new ( &asStr() ) StrType( s );
    }
    DynNode( NodeType type = VEC_NODE_TYPE )
    {
        nodeType = type;
        switch ( type )
        {
        case STR_NODE_TYPE:
            new ( &asStr() ) StrType();
            return;
        case MAP_NODE_TYPE:
            new ( &asMap() ) MapType();
            return;
        case VEC_NODE_TYPE:
            new ( &asVec() ) VecType();
            return;
        }
    }
    DynNode( this_type &&a )
    {
        nodeType = a.nodeType;
        switch ( nodeType )
        {
        case STR_NODE_TYPE:
            new ( &asStr() ) StrType( std::move( a.asStr() ) );
            return;
        case MAP_NODE_TYPE:
            new ( &asMap() ) MapType( std::move( a.asMap() ) );
            return;
        case VEC_NODE_TYPE:
            new ( &asVec() ) VecType( std::move( a.asVec() ) );
            return;
        }
    }
    ~DynNode()
    {
        switch ( nodeType )
        {
        case STR_NODE_TYPE:
            asStr().~StrType();
            return;
        case MAP_NODE_TYPE:
            asMap().~MapType();
            return;
        case VEC_NODE_TYPE:
            asVec().~VecType();
            return;
        }
    }
    NodeType getNodeType() const
    {
        return nodeType;
    }
    void resetToMap()
    {
        this->~this_type();
        new ( this ) DynNode( MAP_NODE_TYPE );
    }
    void resetToVec()
    {
        this->~this_type();
        new ( this ) DynNode( VEC_NODE_TYPE );
    }
    void resetToStr( const StrType &s )
    {
        this->~this_type();
        new ( this ) DynNode( s );
    }

    this_type &operator=( this_type &&a )
    {
        this->~this_type();
        new ( this ) this_type( std::move( a ) );
        return *this;
    }

    this_type deepcopy() const
    {
        if ( nodeType == STR_NODE_TYPE )
        {
            return this_type( asStr() );
        }
        this_type root( nodeType );
        if ( nodeType == VEC_NODE_TYPE )
        {
            for ( const auto &pNode : asVec() )
            {
                assert( pNode );
                root.vecAppend( pNode->deepcopy() );
            }
            return root;
        }
        else
        {
            for ( const auto &kv : asMap() )
            {
                assert( kv.second );
                root.mapInsert( kv.first, kv.second->deepcopy() );
            }
        }
        return root;
    }

    void swap( this_type &node )
    {
        this_type tmp( std::move( node ) );
        node = std::move( *this );
        *this = std::move( tmp );
    }

    // as a VecType, append an element.
    bool vecAppend( this_type &&node )
    {
        assert( nodeType == VEC_NODE_TYPE );
        asVec().push_back( makeNodePtr( std::move( node ) ) );
        return true;
    }
    // as a MapType, append an element.
    bool mapInsert( const StrType &key, this_type &&node, bool bForceUpdate = false )
    {
        return mapInsert( key, makeNodePtr( std::move( node ) ), bForceUpdate );
    }

    StrType &str()
    {
        if ( nodeType != STR_NODE_TYPE )
            throw std::runtime_error( "Expected STR_NODE_TYPE" );
        return asStr();
        ;
    }
    // throws runtime_error if it's not a str
    const StrType &str() const
    {
        if ( nodeType != STR_NODE_TYPE )
            throw std::runtime_error( "Expected STR_NODE_TYPE" );
        return asStr();
    }

    int toInt() const
    {
        auto x = std::strtol( str().c_str(), nullptr, 10 );
        return x;
    }

    double toDouble() const
    {
        auto x = std::strtod( str().c_str(), nullptr );
        return x;
    }
    bool toBool() const
    {
        const auto &s = str();
        if ( iequals( s, "false" ) || iequals( s, "f" ) || iequals( s, "n" ) || iequals( s, "no" ) || iequals( s, "0" ) )
            return false;
        return true;
    }
    // throws runtime_error if it's not a vector
    // throw our_of_range
    this_type &operator[]( size_t idx )
    {
        if ( nodeType != VEC_NODE_TYPE )
        {
            throw std::runtime_error( "Expected VEC_NODE_TYPE" );
        }
        return *asVec().at( idx );
    }
    // throws runtime_error if it's not a map
    // throw our_of_range
    this_type &operator[]( const StrType &key )
    {
        if ( nodeType != MAP_NODE_TYPE )
            throw std::runtime_error( "Expected MAP_NODE_TYPE" );
        return *asMap().at( key );
    }
    // throw our_of_range
    const this_type &operator[]( size_t idx ) const
    {
        if ( nodeType != VEC_NODE_TYPE )
            throw std::runtime_error( "Expected VEC_NODE_TYPE" );
        return *asVec().at( idx );
    }
    // throw our_of_range
    const this_type &operator[]( const StrType &key ) const
    {
        if ( nodeType != MAP_NODE_TYPE )
            throw std::runtime_error( "Expected MAP_NODE_TYPE" );
        return *asMap().at( key );
    }

    // call func( key, DynNode ) for each element in map.
    template<class Func>
    void mapForeach( Func &&func )
    {
        if ( nodeType != MAP_NODE_TYPE )
            throw std::runtime_error( "Expected MAP_NODE_TYPE" );
        for ( auto &kv : asMap() )
            func( kv.first, *kv.second );
    }

    template<class Func>
    void mapForeach( Func &&func ) const
    {
        if ( nodeType != MAP_NODE_TYPE )
            throw std::runtime_error( "Expected MAP_NODE_TYPE" );
        for ( const auto &kv : asMap() )
            func( kv.first, *kv.second );
    }

    // call func( DynNode ) for each element in vec.
    template<class Func>
    void vecForeach( Func &&func )
    {
        if ( nodeType != VEC_NODE_TYPE )
            throw std::runtime_error( "Expected VEC_NODE_TYPE" );
        for ( auto &v : asVec() )
            func( *v );
    }

    template<class Func>
    void vecForeach( Func &&func ) const
    {
        if ( nodeType != VEC_NODE_TYPE )
            throw std::runtime_error( "Expected VEC_NODE_TYPE" );
        for ( const auto &v : asVec() )
            func( *v );
    }

    bool mapContains( const StrType &key ) const
    {
        if ( nodeType != MAP_NODE_TYPE )
            throw std::runtime_error( "Expected MAP_NODE_TYPE" );
        return asMap().count( key );
    }

    // @return the mapped node with key equals given key value.
    // if no such nod found, throws out_of_range.
    // if current is a vector, search the child elements and return mapped node.
    // if current is a map, search the keys and return the mapped node.
    const this_type &childWithKey( const StrType &key ) const
    {
        if ( nodeType == VEC_NODE_TYPE )
        {
            for ( const auto &v : asVec() )
            {
                if ( v->getNodeType() == MAP_NODE_TYPE && v->asMap().count( key ) )
                    return *v->asMap()[key];
            }
        }
        else if ( nodeType == MAP_NODE_TYPE )
        {
            return ( *this )[key];
        }
        else
        {
            throw std::runtime_error( "Expected VEC_NODE_TYPE, MAP_NODE_TYPE" );
        }
        throw std::out_of_range( key );
    }

    // similar to above. But return the map that contains the key value pair.
    // @return the child node with key equals given key value.
    // if no such nod found, throws out_of_range.
    // if current is a vector, search the child elements and return the child node.
    // if current is a map, search the mapped values of children and return mapped value.
    const this_type &childWithKeyValue( const StrType &key, const StrType &val ) const
    {
        if ( nodeType == VEC_NODE_TYPE )
        {
            for ( const auto &v : asVec() )
            {
                if ( v->getNodeType() == MAP_NODE_TYPE )
                {
                    const auto &m = v->asMap();
                    auto it = m.find( key );
                    if ( m.end() != it && it->second->getNodeType() == STR_NODE_TYPE && it->second->str() == val )
                        return *v;
                }
            }
        }
        else if ( nodeType == MAP_NODE_TYPE )
        {
            for ( const auto &v : asMap() )
            {
                if ( v.second->getNodeType() == MAP_NODE_TYPE )
                {
                    const auto &m = v.second->asMap();
                    auto it = m.find( key );
                    if ( m.end() != it && it->second->getNodeType() == STR_NODE_TYPE && it->second->str() == val )
                        return *v.second;
                }
            }
        }
        else
        {
            throw std::runtime_error( "Expected VEC_NODE_TYPE, MAP_NODE_TYPE" );
        }
        throw std::out_of_range( key );
    }

    // return 1 if it's a string type.
    size_t size() const
    {
        if ( nodeType == VEC_NODE_TYPE )
            return asVec().size();
        if ( nodeType == MAP_NODE_TYPE )
            return asMap().size();
        return 1;
    }

    static bool iequals( const std::string &a, const std::string &b )
    {
        unsigned int sz = a.size();
        if ( b.size() != sz )
            return false;
        for ( unsigned int i = 0; i < sz; ++i )
            if ( std::tolower( a[i] ) != std::tolower( b[i] ) )
                return false;
        return true;
    }

protected:
    DynNodePtr makeNodePtr( this_type &&node )
    {
        return DynNodePtr( new this_type( std::move( node ) ) );
    }
    StrType &asStr()
    {
        void *p = m_data;
        return *reinterpret_cast<StrType *>( p );
    }
    const StrType &asStr() const
    {
        const void *p = m_data;
        return *reinterpret_cast<const StrType *>( p );
    }
    VecType &asVec()
    {
        void *p = m_data;
        return *reinterpret_cast<VecType *>( p );
    }
    const VecType &asVec() const
    {
        const void *p = m_data;
        return *reinterpret_cast<const VecType *>( p );
    }
    MapType &asMap()
    {
        void *p = m_data;
        return *reinterpret_cast<MapType *>( p );
    }
    const MapType &asMap() const
    {
        const void *p = m_data;
        return *reinterpret_cast<const MapType *>( p );
    }

    bool mapInsert( const StrType &key, DynNodePtr &&pNode, bool bForceUpdate = false )
    {
        assert( nodeType == MAP_NODE_TYPE );
        auto &m = asMap();
        auto it = m.find( key );
        if ( it != m.end() )
        {
            if ( !bForceUpdate )
                return false;
            it->second = std::move( pNode );
            return true;
        }
        m.insert( std::make_pair( key, std::move( pNode ) ) );
        return true;
    }

protected:
    static const size_t DATASIZE = STR_SIZE > MAP_SIZE ? ( STR_SIZE > VEC_SIZE ? STR_SIZE : VEC_SIZE )
                                                       : ( MAP_SIZE > VEC_SIZE ? MAP_SIZE : VEC_SIZE );
    NodeType nodeType;
    char m_data[DATASIZE]; // may use std::variant when updated to c++17.
};
using JsonNode = DynNode<>;

template<size_t unit = 4, bool newLine = true>
struct Indent
{
    int level;
    Indent( int level = 0 ) : level( level )
    {
    }
    friend std::ostream &operator<<( std::ostream &os, Indent ind )
    {
        if ( ind.level < 0 ) // no print for negative level
            return os;
        if ( newLine )
            os << std::endl;
        for ( int i = 0; i < ind.level; ++i )
        {
            os << "    ";
        }
        return os;
    }
};
using Indent4 = Indent<>;
using Indent2 = Indent<2>;

struct JsonGrammar
{
    static constexpr char ELEMSEP = ',', KVSEP = ':', VECLB = '[', VECRB = ']', MAPLB = '{', MAPRB = '}', NEWLINE = '\n';
    static constexpr const char *COMMENT = "//"; // must be 2 chars. set to '\0' if absent.

    enum class TokenType
    {
        NONE,
        QUOTE,
        MAP_START,
        MAP_END,
        VEC_START,
        VEC_END,
        KV_SEP,
        DELIM,
        ID, // alphnum
        NEW_LINE,
        ESCAPE,
        FILE_END,
        SPACE,
        INVALID
    };
    struct Token
    {
        TokenType toktype;
        int ch;

        bool valid() const
        {
            return toktype != TokenType::NONE && toktype != TokenType::INVALID;
        }
    };
    struct Pos
    {
        int line = 0, col = 0;

        void advance()
        {
            ++col;
        }
        void newline()
        {
            ++line;
            col = 0;
        }
        friend std::ostream &operator<<( std::ostream &os, const Pos &pos )
        {
            os << "line: " << pos.line << ", col: " << pos.col;
            return os;
        }
    };
    static Token get_token( int c )
    {
        switch ( c )
        {
        case ',':
            return {TokenType::DELIM, c};
        case ':':
            return {TokenType::KV_SEP, c};
        case '{':
            return {TokenType::MAP_START, c};
        case '}':
            return {TokenType::MAP_END, c};
        case '[':
            return {TokenType::VEC_START, c};
        case ']':
            return {TokenType::VEC_END, c};
        case '\n':
            return {TokenType::NEW_LINE, c};
            //-- above are grammar chars.
        case '\\':
            return {TokenType::ESCAPE, c};
        case '"':
        case '\'':
            return {TokenType::QUOTE, c};
        case EOF:
            return {TokenType::FILE_END, EOF};
        default:
            if ( isspace( c ) )
                return {TokenType::SPACE, c};
            if ( std::isprint( c ) )
                return {TokenType::ID, c};
            return {TokenType::INVALID, c};
        }
    }
};

struct JsonTrait
{
    static bool is_grammar_char( int c )
    {
        return c == JsonGrammar::ELEMSEP || c == JsonGrammar::KVSEP || c == JsonGrammar::VECLB || c == JsonGrammar::VECRB ||
               c == JsonGrammar::MAPLB || c == JsonGrammar::MAPRB;
    }

    static bool is_identifier( int c )
    {
        return std::isprint( c );
    }

    static JsonGrammar::Token skip_till_token( std::istream &ss, JsonGrammar::Pos &pos )
    {
        while ( ss )
        {
            pos.advance();
            auto tok = JsonGrammar::get_token( ss.get() );
            if ( JsonGrammar::COMMENT[0] && tok.ch == JsonGrammar::COMMENT[0] )
            { // check line comment "//"
                if ( JsonGrammar::COMMENT[1] && JsonGrammar::COMMENT[1] == ss.peek() )
                { // skip the whole line.
                    while ( ss && ss.get() != '\n' )
                        ;
                    pos.newline();
                    continue;
                }
            }

            assert( tok.valid() );
            if ( tok.toktype == JsonGrammar::TokenType::NEW_LINE )
            {
                pos.newline();
                continue;
            }
            if ( tok.toktype != JsonGrammar::TokenType::SPACE )
                return tok;
        }
        return {JsonGrammar::TokenType::FILE_END, EOF};
    }
};

template<class GrammarChars = JsonGrammar>
struct JsonSerializer
{
    ///////////////////////////////////////////////////////////////////////////////////
    ///            print
    ///////////////////////////////////////////////////////////////////////////////////

    // set indentLevel == -1 for compact print.
    template<class StrType>
    static std::ostream &printStr( std::ostream &os, const StrType &s )
    {
        if ( s.empty() || std::any_of( s.begin(), s.end(), []( char c ) { return std::isspace( c ); } ) )
        {
            os << '"' << s << '"';
        }
        else
            os << s;
        return os;
    }

    template<class StrT = std::string>
    std::ostream &write( std::ostream &os, const DynNode<StrT> &dyn, int indentLevel = 0 ) const
    {
        using Node = DynNode<StrT>;
        int n = 0, LEN;
        switch ( dyn.getNodeType() )
        {
        case Node::STR_NODE_TYPE:
            os << Indent4( indentLevel );
            printStr( os, dyn.str() );
            break;
        case Node::VEC_NODE_TYPE:
            os << Indent4( indentLevel ) << GrammarChars::VECLB;
            indentLevel = indentLevel < 0 ? indentLevel : indentLevel + 1;
            LEN = dyn.size();
            dyn.vecForeach( [&]( const Node &node ) {
                write( os, node, indentLevel );
                if ( ++n != LEN )
                    os << GrammarChars::ELEMSEP;
            } );
            os << Indent4( indentLevel - 1 ) << GrammarChars::VECRB;
            break;
        case Node::MAP_NODE_TYPE:
            os << Indent4( indentLevel ) << GrammarChars::MAPLB;
            indentLevel = indentLevel < 0 ? indentLevel : indentLevel + 1;
            LEN = dyn.size();
            dyn.mapForeach( [&]( const StrT &key, const Node &node ) {
                os << Indent4( indentLevel );
                printStr( os, key );
                if ( indentLevel >= 0 )
                    os << ' ';
                os << GrammarChars::KVSEP;
                if ( indentLevel >= 0 )
                    os << ' ';
                if ( node.getNodeType() == Node::STR_NODE_TYPE )
                    printStr( os, node.str() );
                else
                    write( os, node, indentLevel );
                if ( ++n != LEN )
                    os << GrammarChars::ELEMSEP;
            } );
            os << Indent4( indentLevel - 1 ) << GrammarChars::MAPRB;
            break;
        }
        return os;
    }
    template<class StrT = std::string>
    std::ostream &printJsonCompact( std::ostream &os, const DynNode<StrT> &dyn ) const
    {
        return printJson( os, dyn, -1 );
    }

    ///////////////////////////////////////////////////////////////////////////////////
    ///            read
    ///////////////////////////////////////////////////////////////////////////////////

    template<class StrT = std::string>
    bool read( DynNode<StrT> &dyn, std::istream &ss, std::ostream &err ) const
    {
        typename GrammarChars::Pos pos{1, 0};
        //        int lineCount = 1, charCount = 0;
        return read_json( dyn, ss, err, pos );
    }

protected:
    // INVALID for error
    // ID for string
    // other for grammar
    typename GrammarChars::Token read_json_str( std::istream &ss, std::string &s, std::ostream &err, typename GrammarChars::Pos &pos ) const
    {
        using my = JsonTrait;
        using TokenType = typename GrammarChars::TokenType;
        s.clear();
        auto c = my::skip_till_token( ss, pos );
        if ( c.toktype == TokenType::FILE_END )
            return c;
        if ( my::is_grammar_char( c.ch ) )
            return c;
        if ( c.toktype == TokenType::QUOTE )
        {
            while ( ss )
            {
                pos.advance();
                auto cc = GrammarChars::get_token( ss.get() );
                if ( c.ch == cc.ch ) // quote ends
                    return {TokenType::ID, cc.ch}; // got string
                if ( cc.toktype == TokenType::ESCAPE ) // escape
                {
                    if ( ss )
                    {
                        cc = GrammarChars::get_token( ss.get() );
                        if ( cc.ch == '\"' || cc.ch == '\'' ) // todo: handles all escape chars.
                            s += char( cc.ch );
                        else
                        {
                            err << "Unable to escape " << char( cc.ch ) << " at " << pos;
                            return {TokenType::INVALID, cc.ch};
                        }
                    }
                    else
                    {
                        err << "EOF when expecting quote: " << c.ch << " at " << pos;
                        return {TokenType::INVALID, EOF};
                    }
                }
                else if ( c.toktype == TokenType::NEW_LINE )
                {
                    pos.newline();
                    err << "new line within quoted string"
                        << " at " << pos;
                    return {TokenType::INVALID, GrammarChars::NEWLINE};
                }
                else
                {
                    s += char( cc.ch );
                }
            }
            assert( false );
        }
        if ( c.toktype == TokenType::ID ) // isalnum, unquoted string
        {
            s += char( c.ch );
            while ( ss )
            {
                c = GrammarChars::get_token( ss.peek() );
                if ( c.toktype == TokenType::ID ) // check all valid chars
                {
                    s += char( ss.get() );
                    pos.advance();
                }
                else
                    break;
            }
            return {TokenType::ID, 0}; // got string
        }
        assert( false ); // never reach here
        return {TokenType::INVALID, c.ch};
    }

    template<class StrT = std::string>
    bool read_json( DynNode<StrT> &dyn, std::istream &ss, std::ostream &err, typename GrammarChars::Pos &pos ) const
    {
        using Node = DynNode<StrT>;
        using DynStr = typename Node::StrType;
        using TokenType = typename GrammarChars::TokenType;

        std::string s;
        auto res = read_json_str( ss, s, err, pos );
        if ( res.toktype == TokenType::MAP_START ) // read map
        {
            dyn.resetToMap();
            while ( true )
            {
                std::string key;
                res = read_json_str( ss, key, err, pos );
                if ( res.toktype == TokenType::MAP_END ) // end of map
                    return true;
                if ( res.toktype != TokenType::ID )
                {
                    err << " | expecting map key but got " << key << " at " << pos;
                    return false;
                }
                //-- now have key already
                res = read_json_str( ss, s, err, pos );
                if ( res.toktype != TokenType::KV_SEP )
                {
                    err << " | expecting ':' but got " << s << " for key " << key << " at " << pos;
                    return false;
                }
                Node child;
                auto r = read_json( child, ss, err, pos );
                if ( !r )
                {
                    err << " | error reading value for key=" << key << " at " << pos;
                    return false;
                }
                dyn.mapInsert( DynStr( key ), std::move( child ) );

                res = read_json_str( ss, key, err, pos );
                if ( res.toktype == TokenType::DELIM )
                    continue;
                if ( res.toktype == TokenType::MAP_END ) // end of map
                    return true;
                err << " expecting map end or " << GrammarChars::ELEMSEP << ", but got " << key << " at " << pos;
                return false;
            }
        }
        else if ( res.toktype == TokenType::VEC_START ) // read vec
        {
            dyn.resetToVec();
            while ( true )
            {
                Node child;
                auto r = read_json( child, ss, err, pos );
                if ( !r )
                {
                    err << " | error reading vec value at " << pos;
                    return false;
                }
                dyn.vecAppend( std::move( child ) );
                res = read_json_str( ss, s, err, pos );
                if ( res.toktype == TokenType::VEC_END )
                    return true;
                if ( res.toktype == TokenType::DELIM )
                    continue;
            }
        }
        else if ( res.toktype == TokenType::ID ) // read string
        {
            dyn.resetToStr( s );
            return true;
        }
        //        else
        err << " | expecting map or vec or string at " << pos;
        return false;
    }
};
static constexpr JsonSerializer<> jsonSerializer{};

template<class StrT>
inline std::ostream &operator<<( std::ostream &os, const jz::DynNode<StrT> &node )
{
    return jz::jsonSerializer.write( os, node );
}

/********************************************************************************
 Extented JSON-like jzon format

-- omit ':' between key and value,
-- newline is treated as comma ','  when it's end of key-value pair.
-- duplicate keys:  combine into a single k-v with value of array of all values
-- in array, omit [] and comma ',' if its elements are strings/atomics". note that ',' must be omitted if [] is NOT needed.
-- comments start with '//'.
---------------------------------------------------------------------------------

// document is a map by default, but user can also interpret it as a vector.
{  // this a map
    name John A B                      // name : ["John", "A", "B"]
    scores [ math 23,  english 32, history 123] // [ scores: [ [math 23], [english, 32], [history 123]]
    send [ [John 32] [tim asdf] ]   // send: [ [John 32] [tim asdf] ]
    addr 2 3 4
    addr 5 6 7                               // values are combined into an array : addr:[ [2,3,4], [5, 6, 7] ]
    friends [ "John B" { age 23 }, "Tom A" { age 32 } ]  // friends: [ ["John B", {age: 23}], ...]
    addresses { 1 "2 main st", 2 "3 rock st"}  // addresses : { 1: "2 main st", ...}
    comment "multi-\
line \
text"
},
2 3 4 5 // [2, 3, 4, 5]

***********************************************************************************/
/////////////////////////////////////////////////////////////////////////////////////
///
///                   Jzon format read
///
////////////////////////////////////////////////////////////////////////////////////

template<bool bOmitKVSep = true, bool bNewLineAsComma = true, bool bMultiLineStr = true, bool bCombineDupKeys = true, bool bOmitVecBrackets = false>
struct JzonSerializer
{
    template<class StrT = std::string>
    bool read( DynNode<StrT> &dyn, std::istream &ss, std::ostream &err ) const
    {
        typename JsonGrammar::Pos pos{1, 0};
        //        int lineCount = 1, charCount = 0;
        return read_json( dyn, ss, err, pos );
    }

protected:
    struct JzonTrait
    {
        static bool is_grammar_char( int c )
        {
            return JsonTrait::is_grammar_char( c ); // new line as grammar
        }
        static JsonGrammar::Token skip_till_token( std::istream &ss, JsonGrammar::Pos &pos, bool skipNewLine = true )
        {
            while ( ss )
            {
                pos.advance();
                auto tok = JsonGrammar::get_token( ss.get() );
                if ( JsonGrammar::COMMENT[0] && tok.ch == JsonGrammar::COMMENT[0] )
                { // check line comment "//"
                    if ( JsonGrammar::COMMENT[1] && JsonGrammar::COMMENT[1] == ss.peek() )
                    { // skip the whole line.
                        while ( ss && ss.get() != '\n' )
                            ;
                        pos.newline();
                        continue;
                    }
                }

                assert( tok.valid() );
                if ( tok.toktype == JsonGrammar::TokenType::NEW_LINE )
                {
                    pos.newline();
                    if ( skipNewLine )
                        continue;
                    return tok;
                }
                if ( tok.toktype != JsonGrammar::TokenType::SPACE )
                    return tok;
            }
            return {JsonGrammar::TokenType::FILE_END, EOF};
        }
    };
    using JzonGrammar = JsonGrammar;
    // INVALID for error
    // ID for string
    // other for grammar
    typename JsonGrammar::Token read_json_str(
            std::istream &ss, std::string &s, std::ostream &err, typename JzonGrammar::Pos &pos, bool skipNewLine = true ) const
    {
        using my = JzonTrait;
        using TokenType = typename JzonGrammar::TokenType;
        s.clear();
        auto c = my::skip_till_token( ss, pos, skipNewLine );
        if ( c.toktype == TokenType::FILE_END )
            return c;
        if ( my::is_grammar_char( c.ch ) )
            return c;
        if ( !skipNewLine && c.toktype == TokenType::NEW_LINE )
            return c;
        if ( c.toktype == TokenType::QUOTE )
        {
            while ( ss )
            {
                pos.advance();
                auto cc = JzonGrammar::get_token( ss.get() );
                if ( c.ch == cc.ch ) // quote ends
                    return {TokenType::ID, cc.ch}; // got string
                if ( cc.toktype == TokenType::ESCAPE ) // escape
                {
                    if ( ss )
                    {
                        cc = JzonGrammar::get_token( ss.get() );
                        if ( cc.ch == '\"' || cc.ch == '\'' ) // todo: handles all escape chars.
                            s += char( cc.ch );
                        else
                        {
                            err << "Unable to escape " << char( cc.ch ) << " at " << pos;
                            return {TokenType::INVALID, cc.ch};
                        }
                    }
                    else
                    {
                        err << "EOF when expecting quote: " << c.ch << " at " << pos;
                        return {TokenType::INVALID, EOF};
                    }
                }
                else if ( cc.toktype == TokenType::NEW_LINE )
                {
                    pos.newline();
                    if constexpr ( bMultiLineStr )
                    {
                        s += '\n';
                    }
                    else
                    {
                        err << "new line within quoted string"
                            << " at " << pos;
                        return {TokenType::INVALID, JzonGrammar::NEWLINE};
                    }
                }
                else
                {
                    s += char( cc.ch );
                }
            }
            assert( false );
        }
        if ( c.toktype == TokenType::ID ) // isalnum, unquoted string
        {
            s += char( c.ch );
            while ( ss )
            {
                c = JzonGrammar::get_token( ss.peek() );
                if ( c.toktype == TokenType::ID ) // check all valid chars
                {
                    s += char( ss.get() );
                    pos.advance();
                }
                else
                    break;
            }
            return {TokenType::ID, 0}; // got string
        }

        assert( false ); // never reach here
        return {TokenType::INVALID, c.ch};
    }
    /// \param dyn is already constructed as map, read all map elements.
    template<class StrT = std::string>
    bool read_json_map( DynNode<StrT> &dyn, std::istream &ss, std::ostream &err, typename JzonGrammar::Pos &pos ) const
    {
        std::string s;
        using Node = DynNode<StrT>;
        using DynStr = typename Node::StrType;
        using TokenType = typename JzonGrammar::TokenType;

        std::unordered_set<std::string> bCombinedKeys;
        while ( true )
        {
            std::string key;
            auto res = read_json_str( ss, key, err, pos );
            if ( res.toktype == TokenType::MAP_END ) // end of map
                return true;
            if ( res.toktype != TokenType::ID )
            {
                err << " | expecting map key but got " << key << " at " << pos;
                return false;
            }
            //-- now have key already
            Node child;
            res = read_json_str( ss, s, err, pos, !bNewLineAsComma );
            if ( res.toktype == TokenType::KV_SEP || ( bNewLineAsComma && res.toktype == TokenType::NEW_LINE ) ) // read ':'
            {
                auto r = read_json( child, ss, err, pos );
                if ( !r )
                {
                    err << " | error reading value map for key=" << key << " at " << pos;
                    return false;
                }
            }
            else
            {
                if constexpr ( bOmitKVSep )
                {
                    if ( res.toktype == TokenType::ID )
                    {
                        child.resetToStr( s );
                        // todo: forwad read next str.
                        // if its ID and bOmitVecBrackets, construct into a vec.
                    }
                    else if ( res.toktype == TokenType::VEC_START )
                    {
                        child.resetToVec();
                        auto r = read_json_vec( child, ss, err, pos );
                        if ( !r )
                        {
                            err << " | error reading value vec for key=" << key << " at " << pos;
                            return false;
                        }
                    }
                    else if ( res.toktype == TokenType::MAP_START )
                    {
                        child.resetToMap();
                        auto r = read_json_map( child, ss, err, pos );
                        if ( !r )
                        {
                            err << " | error reading value vec for key=" << key << " at " << pos;
                            return false;
                        }
                    }
                    else
                    {
                        err << " | expecting ':' but got " << s << " for key " << key << " at " << pos;
                        return false;
                    }
                }
                else
                {
                    err << " | expecting ':' but got " << s << " for key " << key << " at " << pos;
                    return false;
                }
            }

            if ( dyn.mapContains( key ) )
            {
                if constexpr ( bCombineDupKeys )
                {
                    auto &childdyn = dyn[key];
                    if ( !bCombinedKeys.count( key ) ) // it's alreay combined into vec.
                    {
                        bCombinedKeys.insert( key );
                        Node tmp( std::move( childdyn ) );
                        childdyn.resetToVec();
                        childdyn.vecAppend( std::move( tmp ) );
                    }
                    childdyn.vecAppend( std::move( child ) );
                }
                else
                {
                    err << " duplicate key:" << key << " at " << pos;
                    return false;
                }
            }
            else
                dyn.mapInsert( DynStr( key ), std::move( child ) );


            res = read_json_str( ss, key, err, pos, !bNewLineAsComma );
            if ( res.toktype == TokenType::DELIM || ( bNewLineAsComma && res.toktype == TokenType::NEW_LINE ) )
                continue;
            if ( res.toktype == TokenType::MAP_END ) // end of map
                return true;
            err << " expecting map end or " << JzonGrammar::ELEMSEP << ", but got " << key << " at " << pos;
            return false;
        }
    }

    template<class StrT = std::string>
    bool read_json_vec( DynNode<StrT> &dyn, std::istream &ss, std::ostream &err, typename JzonGrammar::Pos &pos, bool bNeedsVecEnd = true ) const
    {
        std::string s;
        using Node = DynNode<StrT>;
        using DynStr = typename Node::StrType;
        using TokenType = typename JzonGrammar::TokenType;
        // bOmitVecBrackets
        Node child;
        while ( true )
        {
            auto res = read_json_str( ss, s, err, pos );
            if constexpr ( bOmitVecBrackets )
            {
                if ( bNeedsVecEnd && res.toktype == TokenType::VEC_END )
                    return true;
            }
            if ( res.toktype == TokenType::VEC_END )
                return true;
            if ( res.toktype == TokenType::ID )
            {
                child.resetToStr( s );
                dyn.vecAppend( std::move( child ) );
                continue;
            }
            else if ( res.toktype == TokenType::DELIM )
                continue;
            else if ( res.toktype == TokenType::VEC_START )
            {
                child.resetToVec();
                if ( !read_json_vec( child, ss, err, pos ) )
                    return false;
                dyn.vecAppend( ( std::move( child ) ) );
            }
            else if ( res.toktype == TokenType::MAP_START )
            {
                child.resetToVec();
                if ( !read_json_vec( child, ss, err, pos ) )
                    return false;
                dyn.vecAppend( ( std::move( child ) ) );
            }

            auto r = read_json( child, ss, err, pos, DynNode<StrT>::VEC_NODE_TYPE );
            if ( !r )
            {
                err << " | error reading vec value at " << pos;
                return false;
            }
        }
    }

    template<class StrT = std::string>
    bool read_json( DynNode<StrT> &dyn,
                    std::istream &ss,
                    std::ostream &err,
                    typename JzonGrammar::Pos &pos,
                    typename DynNode<StrT>::NodeType parentNodeType = DynNode<StrT>::NodeType::NIL_NODE_TYPE ) const
    {
        using Node = DynNode<StrT>;
        using DynStr = typename Node::StrType;
        using TokenType = typename JzonGrammar::TokenType;

        std::string s;
        auto res = read_json_str( ss, s, err, pos );
        if ( res.toktype == TokenType::MAP_START ) // read map
        {
            dyn.resetToMap();
            return read_json_map( dyn, ss, err, pos );
        }
        else if ( res.toktype == TokenType::VEC_START ) // read vec
        {
            dyn.resetToVec();
            return read_json_map( dyn, ss, err, pos );
        }
        else if ( res.toktype == TokenType::ID ) // read string
        {
            dyn.resetToStr( s );
            return true;
        }
        //        else
        err << " | expecting map or vec or string at " << pos;
        return false;
    }
}; // namespace jz
JzonSerializer<> jzonSerializer{};
} // namespace jz
