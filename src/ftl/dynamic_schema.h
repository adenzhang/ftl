#pragma once
#include <ftl/stdconcept.h>
#include <ftl/dynamic.h>
#include <unordered_map>
#include <vector>
#include <sstream>

namespace ftl
{


template<class T, class MemberType>
struct MemberAccess
{
    using member_type = MemberType;
    MemberType T::*pMember;
    const char *name;

    void operator()( T &obj, MemberType val ) const
    {
        obj.*pMember = std::move( val );
    }
};

template<class T, class MemberType>
struct MemberSetter
{
    using member_type = MemberType;
    void ( T::*pSetMember )( MemberType );
    const char *name;

    void operator()( T &obj, MemberType val ) const
    {
        obj.*pSetMember( std::move( val ) );
    }
};

template<class T, class MemberType>
struct MemberSetterFunc
{
    using member_type = MemberType;
    void ( *pFunc )( T &, MemberType val );
    const char *name;

    void operator()( T &obj, MemberType val ) const
    {
        ( *pFunc )( obj, std::move( val ) );
    }
};

template<class T, class MemberType>
MemberAccess<T, MemberType> make_member_setter( MemberType T::*pMember, const char *name )
{
    return MemberAccess<T, MemberType>{pMember, name};
}

template<class T, class MemberType>
MemberSetter<T, MemberType> make_member_setter( void ( T::*pSetMember )( const MemberType & ), const char *name )
{
    return MemberSetter<T, MemberType>{pSetMember, name};
}

template<class T, class MemberType>
MemberSetterFunc<T, MemberType> make_member_setter( void ( *pFunc )( T &, const MemberType &val ), const char *name )
{
    return MemberSetterFunc<T, MemberType>{pFunc, name};
}


FTL_CHECK_TYPE( HasMember_value_type, T::value_type );
FTL_CHECK_TYPE( HasMember_key_type, T::key_type );
FTL_CHECK_TYPE( HasMember_mapped_type, T::mapped_type );
FTL_HAS_MEMBER( HasMember_push_back, push_back( 0 ) );
FTL_HAS_MEMBER( HasMember_operator_sb, operator[]( 0 ) ); // square brackets
FTL_HAS_MEMBER( HasMember_insert, insert( 0 ) );
FTL_HAS_MEMBER( HasMember_c_str, c_str() );
FTL_HAS_MEMBER( HasMember_get_children_schema, get_children_schema );

template<class T>
static constexpr bool IsLikeMap_v = HasMember_key_type<T>::value &&HasMember_mapped_type<T>::value &&HasMember_insert<T>::value;

template<class T>
static constexpr bool IsLikeVec_v = HasMember_value_type<T>::value && !HasMember_key_type<T>::value && HasMember_push_back<T>::value &&
                                    !HasMember_c_str<T>::value; // not a string

using DynStr = std::string;
using DynMap = dynamic_map<std::string, DynStr>;
using DynVec = dynamic_vec<std::string, DynStr>;
using DynVar = dynamic_var<std::string, DynStr>;

template<class T, class Str>
bool from_string( T val, const Str &s )
{
    assert( "not implemented" );
    throw std::runtime_error( "from_string not implemented!" );
    return false;
}
template<class T, class Str, class Err>
bool convert_from_string( T &t, const Str &s, Err &err )
{
    if constexpr ( std::is_constructible_v<T, Str> )
    {
        new ( &t ) T( s );
        return true;
    }
    else
    {
        if ( !from_string( t, s ) )
        {
            err << " ERROR! Failed to convert from string:" << s;
            return false;
        }
        return true;
    }
}

template<class Schema, class Dyn>
bool parse_schema( Schema &schema, const Dyn &dyn, std::ostream &err )
{
    if ( dyn.is_map() )
    {
        if constexpr ( IsLikeMap_v<Schema> ) // a map
        {
            bool ok = true;
            dyn.foreach_map_entry( [&]( const auto keystr, const auto &value ) {
                if ( !ok )
                    return;
                typename Schema::key_type key;
                if ( !convert_from_string( key, keystr ) )
                {
                    err << " | key:" << keystr;
                    ok = false;
                    return;
                }
                typename Schema::mapped_type val;
                if ( !parse_schema( val, value, err ) ) // recursive parse
                {
                    err << " | key:" << keystr;
                    ok = false;
                    return;
                }
                auto res = schema.insert( Schema::value_type( key, val ) );
                if ( !res.second )
                {
                    err << " ERROR! Failed to insert into map. key: " << key << ", oldvalue: " << res.first->second << ", newvalue: " << val;
                    ok = false;
                }
            } );
            return ok;
        }
        else if constexpr ( HasMember_get_children_schema<Schema>::value ) // an object
        {
            bool ok = true;
            dyn.foreach_map_entry( [&]( const auto &key, const auto &value ) {
                if ( !ok )
                    return;
                const auto &childSchemas = Schema::get_children_schema();
                bool bProcessed = false;
                tuple_foreach( childSchemas, [&]( const auto &memberSetter ) {
                    if ( !ok || bProcessed || key != memberSetter.name )
                        return;
                    bProcessed = true;
                    using ChildSchemaType = typename std::remove_reference_t<decltype( memberSetter )>::member_type;
                    ChildSchemaType childSchema;
                    if ( !parse_schema( childSchema, value, err ) ) // recursive parse
                    {
                        err << " | key: " << key;
                        ok = false;
                        return;
                    }
                    memberSetter( schema, std::move( childSchema ) );
                    return;
                } );

                if ( !bProcessed )
                {
                    err << " ERROR! No matched schema key:" << key;
                    ok = false;
                }
            } );
            return ok;
        }
        else // schema is not map or schema class
        {
            err << " ERROR! Expected map or schema type. ";
            return false;
        }
    }
    else if ( dyn.is_vec() ) // vector
    {
        if constexpr ( !IsLikeVec_v<Schema> )
        {
            err << " ERROR! Expected vec schema type.";
            return false;
        }
        else
        {
            bool ok = true;
            dyn.foreach_vec_entry( [&]( const auto &var ) {
                if ( !ok )
                    return;
                typename Schema::value_type val;
                if ( !parse_schema( val, var, err ) ) // recursive parse
                {
                    err << " | array";
                    ok = false;
                    return;
                }
                schema.push_back( val );
            } );
            return ok;
        }
    }
    else // primitive element: string
    {
        auto &str = dyn.as_str();
        if constexpr ( IsLikeVec_v<Schema> )
        {
            err << " ERROR! Unable to parse string to vec: " << str;
            return false;
        }
        if constexpr ( IsLikeMap_v<Schema> )
        {
            err << " ERROR! Unable to parse string to map: " << str;
            return false;
        }
        return convert_from_string( schema, str, err );
    }
}
} // namespace ftl
