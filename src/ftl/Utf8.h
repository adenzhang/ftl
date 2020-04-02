#pragma once
#include <map>

namespace jz
{

template<class UnicodeValueType = int>
struct Utf8
{
    using DecodeResult = std::pair<UnicodeValueType, unsigned>; // <val, len>
    // return <utf8_val, nbytes>. nbytes == 0 if it's not valid encoding.
    static DecodeResult decodeUtf8Char( const unsigned char *s )
    {
        UnicodeValueType val = 0;
        unsigned len = 0;
        if ( ( *s & 0x80 ) == 0 )
        { // one byte
            val = *s;
            len = 1;
        }
        else if ( ( *s & 0xE0 ) == 0xC0 )
        { // 110x xxxx, 2 bytes
            if ( ( *( s + 1 ) & 0xC0 ) == 0x80 )
            { // 10xx xxxx,
                val = ( unsigned( ( *s ) & 0x1F ) << 6 ) | ( *( s + 1 ) & 0x3F ); // 11 bits
                len = 2;
            }
        }
        else if ( ( *s & 0xF0 ) == 0xE0 )
        { // 1110 xxxx, 3 bytes
            if ( ( *( s + 1 ) & 0xC0 ) == 0x80 && ( *( s + 2 ) & 0xC0 ) == 0x80 )
            { // 10xx xxxx 10xx xxxx
                val = ( unsigned( ( *s ) & 0x0F ) << 12 ) | ( ( *( s + 1 ) & 0x3F ) << 6 ) | ( *( s + 2 ) & 0x3F ); // 16 bits
                len = 3;
            }
        }
        else if ( ( *s & 0xF8 ) == 0xF0 )
        { // 1111 0xxx, 4 bytes
            if ( ( *( s + 1 ) & 0xC0 ) == 0x80 && ( *( s + 2 ) & 0xC0 ) == 0x80 && ( *( s + 3 ) & 0xC0 ) == 0x80 )
            { // 10xx xxxx, 10xx xxxx, 10xx xxxx
                val = ( unsigned( ( *s ) & 0x07 ) << 18 ) | ( ( *( s + 1 ) & 0x3F ) << 12 ) | ( ( *( s + 2 ) & 0x3F ) << 6 ) |
                      ( *( s + 3 ) & 0x3F ); // 21 bits
                len = 4;
            }
        }
        return {val, len};
    }

    // return length of bytes encoded
    static unsigned encodeUtf8( char *buf, UnicodeValueType v )
    {
        unsigned char *s = (unsigned char *)buf;
        if ( v < 0 || v >= 1112064 )
            return 0; // runtime
        if ( v < 128 )
        {
            *s++ = v;
            *s = 0;
            return 1;
        }
        if ( v < 2048 )
        { // 2 bytes
            *s++ = 0xC0 | char( v >> 6 );
            *s++ = 0x80 | char( 0x3F & v );
            *s = 0;
            return 2;
        }
        if ( v < 65536 )
        { // 3 bytes
            *s++ = 0xE0 | char( v >> 12 );
            *s++ = 0x80 | char( 0x3F & ( v >> 6 ) );
            *s++ = 0x80 | char( 0x3F & v );
            *s = 0;
            return 3;
        }

        *s++ = 0xF7 | char( v >> 18 );
        *s++ = 0x80 | char( 0x3F & ( v >> 12 ) );
        *s++ = 0x80 | char( 0x3F & ( v >> 6 ) );
        *s++ = 0x80 | char( 0x3F & v );
        *s = 0;
        return 4;
    }

    class InvalidEncode : public std::runtime_error
    {
    public:
        InvalidEncode( const char *s ) : std::runtime_error( s )
        {
        }
    };
    struct Iterator
    {
        using value_type = UnicodeValueType; // unicode value

        const unsigned char *s = nullptr;
        DecodeResult val;

        Iterator() = default;
        Iterator( const char *s ) : s( (const unsigned char *)s )
        {
            val = decodeUtf8Char( this->s );
            if ( !val.second )
                throw InvalidEncode( s );
        }
        Iterator( const Iterator & ) = default;
        Iterator &operator=( const Iterator & ) = default;

        bool operator==( const Iterator &a ) const
        {
            return ( isEnd() && a.isEnd() ) || s == a.s;
        }
        bool operator!=( const Iterator &a ) const
        {
            return !this->operator==( a );
        }

        bool isEnd() const
        {
            return !s || !*s;
        }

        const value_type operator*() const
        {
            return val.first;
        }
        const value_type *operator->() const
        {
            return &val.first;
        }
        Iterator &operator++()
        {
            new ( this ) Iterator( (const char *)s + val.second );
        }
        Iterator operator++( int )
        {
            Iterator a = *this;
            ++( *this );
            return a;
        }
    };

    Utf8( const char *s ) : s( s )
    {
    }

    Iterator begin()
    {
        return {s};
    }
    Iterator end()
    {
        return {};
    }

protected:
    const char *s = nullptr;
};
using Utf8Int = Utf8<int>;
} // namespace jz
