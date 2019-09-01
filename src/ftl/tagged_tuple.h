/*
 * This file is part of the ftl (Fast Template Library) distribution (https://github.com/adenzhang/ftl).
 * Copyright (c) 2019 Aden Zhang.
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

#include <tuple>

/***********************************************
 *
 *

void test_tagged_tuple()
{
    ValueList<'i'> vl{};
    TaggedTuple<Tagged<"name"_tref, std::string>, TaggedType<decltype( "id"_t ), int>, Tagged<"score"_tref, double>> taggedTup;
    taggedTup.get( "name"_t ) = "asdf";
    taggedTup.get( "id"_t ) = 23;

    auto subtup = taggedTup.sub( "name"_t, "id"_t );
    subtup["name"_t] += "+23";

    auto &constup = taggedTup;
    auto subreftup = constup.subref( "name"_t, "id"_t );
    subreftup["name"_t] = "abc";

    std::cout << "name:" << taggedTup["name"_t] << ",  subtuple same name:" << subtup["name"_t] << std::endl;

    auto subsubref = subreftup.subref( "name"_t, "id"_t );
    subsubref["name"_t] = "subsubref";

    std::cout << "name:" << taggedTup["name"_t] << ",  subtuple same name:" << subtup["name"_t] << std::endl;
}
 *
 *
 * ********************************************/

namespace ftl
{

//////////////////////////////////////////////////////////////////////////////////////////////
///
///  TypeList
///
//////////////////////////////////////////////////////////////////////////////////////////////


/// @brief Find index of Target type in type list.
template<bool bAssertIfNotFound, class Target, class Head, class... Tails>
constexpr size_t find_type_index()
{
    if constexpr ( std::is_same_v<Target, Head> )
    {
        return 0;
    }
    else if constexpr ( sizeof...( Tails ) == 0 ) // no match
    {
        static_assert( !bAssertIfNotFound, "Type not found!" );
        return 1;
    }
    else
    {
        return 1 + find_type_index<bAssertIfNotFound, Target, Tails...>();
    }
}

template<bool bAssertIfNotFound, class Target>
constexpr size_t find_type_index()
{
    static_assert( !bAssertIfNotFound, "Type not found!" );
    return 1;
}


template<auto... Values>
struct ValueList
{
};

template<char... Values>
struct CharList
{
    static const char *c_str()
    {
        static const char str[]{Values..., '\0'};
        return str;
    }
};


template<typename T, typename... Types>
static constexpr bool unique_types_v = find_type_index<false, T, Types...>() >= sizeof...( Types );

template<typename... Types>
struct TypeList
{
    static constexpr auto size = sizeof...( Types );
};

template<size_t n, typename Head, typename... Tail>
struct NthType
{
    using type = typename NthType<n - 1, Tail...>::type;
};

template<typename Head, typename... Tail>
struct NthType<0, Head, Tail...>
{
    using type = Head;
};

template<size_t n, typename... Types>
using NthType_t = typename NthType<n, Types...>::type;

template<auto... Values>
static constexpr ValueList<Values...> valueList{};


template<char... Values>
static constexpr CharList<Values...> valueCharList{};


//////////////////////////////////////////////////////////////////////////////////////////////
///
///  TaggedTuple
///
//////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get a Tag (ValueList) value from a string literal.
template<class C, C... CHARS>
constexpr auto operator"" _t() noexcept
{
    return CharList<CHARS...>{};
}

/// @brief Get a referene to Tag (ValueList) value from a string literal.
/// @return reference to Tag type, which is used for auto non-type template parameters.
template<class C, C... CHARS>
constexpr auto &operator"" _tref() noexcept
{
    return valueCharList<CHARS...>;
}

template<class TagT, class ValueT>
struct TaggedType
{
    using TagType = TagT;
    using ValueType = ValueT;
};

template<class TagT, class ValueT>
struct TaggedValue
{
    using TagType = TagT;
    using ValueType = ValueT;

    ValueType value;
};

template<class TagT, class ValueT>
std::ostream &operator<<( std::ostream &os, const TaggedValue<TagT, ValueT> &value )
{
    os << TagT::c_str() << ":" << value.value;
    return os;
}

template<class T>
using RemoveCVRef_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<auto &TagValueRef, class ValueT>
using Tagged = TaggedType<RemoveCVRef_t<decltype( TagValueRef )>, ValueT>;

template<class... TaggedTypes>
struct TaggedTuple;

template<class T, class... Args>
auto PrependTuple_( std::tuple<Args...> ) -> std::tuple<T, Args...>;

template<class TaggedType, class... Args>
auto PrependTuple_( TaggedTuple<Args...> ) -> TaggedTuple<TaggedType, Args...>;

template<class T, class Tuple>
using PrependTuple_t = decltype( PrependTuple_<T>( Tuple{} ) );

template<class... TaggedTypes>
struct TaggedTuple
{
    static_assert( unique_types_v<typename TaggedTypes::TagType...>, "Type Conflicts!" );

    using TagTuple = std::tuple<typename TaggedTypes::TagType...>;
    using ValueTuple = std::tuple<typename TaggedTypes::ValueType...>;

    TaggedTuple() = default;

    template<class... ValueTypes>
    explicit TaggedTuple( ValueTypes &&... values ) : tup_{std::forward<ValueTypes>( values )...}
    {
    }

    auto &get_value_tuple()
    {
        return tup_;
    }
    const auto &get_value_tuple() const
    {
        return tup_;
    }

    template<class Tag>
    auto &get()
    {
        return std::get<find_type_index<true, Tag, typename TaggedTypes::TagType...>()>( tup_ );
    }

    template<class Tag>
    const auto &get() const
    {
        return std::get<find_type_index<true, Tag, typename TaggedTypes::TagType...>()>( tup_ );
    }

    template<class Tag>
    auto &operator[]( Tag )
    {
        return get<Tag>();
    }

    template<class Tag>
    const auto &operator[]( Tag ) const
    {
        return get<Tag>();
    }

    template<size_t idx>
    auto &at()
    {
        return std::get<idx>( tup_ );
    }

    template<size_t idx>
    const auto &at() const
    {
        return std::get<idx>( tup_ );
    }

    template<size_t i>
    auto tagged_value_at() const
    {
        using TagType = RemoveCVRef_t<decltype( std::get<i>( std::declval<TagTuple>() ) )>;
        using ValueType = std::remove_reference_t<decltype( std::get<i>( std::declval<ValueTuple>() ) )>;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    template<size_t i>
    auto tagged_value_ref_at()
    {
        using TagType = RemoveCVRef_t<decltype( std::get<i>( std::declval<TagTuple>() ) )>;
        using ValueType = std::remove_reference_t<decltype( std::get<i>( std::declval<ValueTuple>() ) )> &;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    template<size_t i>
    auto tagged_value_ref_at() const
    {
        using TagType = RemoveCVRef_t<decltype( std::get<i>( std::declval<TagTuple>() ) )>;
        using ValueType = const std::remove_reference_t<decltype( std::get<i>( std::declval<ValueTuple>() ) )> &;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    /// @brief Subset of TaggedTuple
    template<class... Tags>
    auto sub( Tags... ) const
    {
        using ReturnType = TaggedTuple<decltype( tagged_value_at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>() )...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Reference to const subset of TaggedTuple
    template<class... Tags>
    auto subref( Tags... ) const
    {
        using ReturnType = TaggedTuple<decltype( tagged_value_ref_at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>() )...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Reference to  subset of TaggedTuple
    template<class... Tags>
    auto subref( Tags... )
    {
        using ReturnType = TaggedTuple<decltype( tagged_value_ref_at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>() )...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Concatinate with another TaggedTuple
    /// @pre No TagType conflicts.
    template<class... TaggedTypes1>
    auto concat( const TaggedTuple<TaggedTypes1...> &another ) const
    {
        static_assert( unique_types_v<typename TaggedTypes::TagType..., typename TaggedTypes1::TagType...>, "Type Conflicts!" );

        using ReturnType = TaggedTuple<TaggedTypes..., TaggedTypes1...>;
        return ReturnType{get<typename TaggedTypes::TagType>()..., another.template get<typename TaggedTypes1::TagType>()...};
    }

    /// @brief for each element, call Function func(Tag, Value)
    template<class F>
    void for_each( const F &func )
    {
        for_each_( func, std::make_index_sequence<sizeof...( TaggedTypes )>(), tup_ );
    }

    template<class F>
    void for_each( const F &func ) const
    {
        for_each_( func, std::make_index_sequence<sizeof...( TaggedTypes )>(), tup_ );
    }

    std::ostream &print_json( std::ostream &os ) const
    {
        os << "{";
        for_each( [&os]( auto tag, const auto &value ) {
            using Tag = decltype( tag );
            constexpr size_t idx = find_type_index<true, Tag, typename TaggedTypes::TagType...>();
            if constexpr ( idx != 0 )
            {
                os << ",";
            }
            os << Tag::c_str() << ":" << value;
        } );
        os << "}";
        return os;
    }

private:
    template<class F, std::size_t... Is, class Tuple>
    static void for_each_( const F &func, std::index_sequence<Is...>, Tuple &&tup )
    {
        using expander = int[];
        (void)expander{0, ( (void)func( NthType_t<Is, typename TaggedTypes::TagType...>{}, std::get<Is>( tup ) ), 0 )...};
    }


    ValueTuple tup_;
};

template<class T, class... TaggedTypes>
std::ostream &operator<<( std::ostream &os, const TaggedTuple<T, TaggedTypes...> &tup )
{
    return tup.print_json( os );
}

} // namespace ftl
