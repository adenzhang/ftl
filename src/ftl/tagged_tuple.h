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
#include <ftl/container_serialization.h>

/***********************************************
 *
 *

TEST_FUNC( tagged_tuple_tests )
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

        std::cout << "combined df2 3 rows:" << df2 << std::endl;

        std::cout << "original df1 3 rows:" << df1 << std::endl;

        std::cout << "df1 is still 2 row: no change:" << df << std::endl;


        { //// test stack
            DataFrame<Tagged<"name"_tref, std::string>, Tagged<"id"_tref, int>> df( std::vector<std::string>{"A", "B"}, std::vector<int>{1, 2} );
            df.stack( TaggedTuple<Tagged<"name"_tref, std::string>, Tagged<"age"_tref, int>, Tagged<"id"_tref, int>>{std::string( "C" ), 23, 3} );

            std::cout << "stacked dataframe 3 rows:" << df << std::endl;

            DataFrame<Tagged<"name"_tref, std::string>, Tagged<"age"_tref, int>, Tagged<"id"_tref, int>> df1(
                    std::vector<std::string>{"D", "E"}, std::vector<int>{20, 50}, std::vector<int>{5, 2} );

            df.stack( df1 );

            std::cout << "stacked dataframe 5 rows:" << df << std::endl;

            auto stacked_copy = df.stack( df1 );

            std::cout << "stacked copy dataframe 7 rows:" << stacked_copy << std::endl;
        }
    }
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

using namespace serialization;

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


enum class ValueRefType
{
    AsIs,
    NoRef,
    Ref,
    ConstRef
};

template<ValueRefType refType, class ValueType>
using RefValueType_t =
        std::conditional_t<refType == ValueRefType::Ref,
                           std::remove_reference_t<ValueType> &,
                           std::conditional_t<refType == ValueRefType::ConstRef,
                                              const std::remove_reference_t<ValueType> &,
                                              std::conditional_t<refType == ValueRefType::NoRef, std::remove_reference_t<ValueType>, ValueType>>>;

template<typename T>
static constexpr ValueRefType RefValueType_v =
        std::is_same_v<T, RefValueType_t<ValueRefType::ConstRef, T>>
                ? ValueRefType::ConstRef
                : ( std::is_same_v<T, RefValueType_t<ValueRefType::Ref, T>>
                            ? ValueRefType::Ref
                            : ( std::is_same_v<T, RefValueType_t<ValueRefType::NoRef, T>> ? ValueRefType::NoRef : ValueRefType::AsIs ) );

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

template<class T>
const T &min_value( const T &a, const T &b )
{
    return std::min( a, b );
}

template<class T, class... Args>
const T &min_value( const T &a, const T &b, const Args &... args )
{
    return min_value( min_value( a, b ), args... );
}

template<class T>
const T &max_value( const T &a, const T &b )
{
    return std::max( a, b );
}
template<class T, class... Args>
const T &max_value( const T &a, const T &b, const Args &... args )
{
    return max_value( max_value( a, b ), args... );
}


template<class U>
struct is_random_accessible_container
{
    template<class T>
    static auto check( int )
            -> decltype( std::declval<typename T::value_type>, std::declval<T>()[0], std::declval<T>().begin(), std::declval<T>().end(), bool() );

    template<class T>
    static int check( ... );

    template<class Char, class Traits>
    static bool check_string( std::basic_string<Char, Traits> );

    template<class Char, class Traits>
    static bool check_string( std::basic_string_view<Char, Traits> );

    template<class T>
    static int check_string( T );

    static constexpr bool is_ra_container = std::is_same_v<decltype( check<U>( 0 ) ), bool>;

    static constexpr bool is_string = std::is_same_v<decltype( check_string( std::declval<U>() ) ), bool>;

    static constexpr bool value = is_ra_container && !is_string;
};

template<class... Types>
static constexpr bool can_construct_dataframe()
{
    return ( true && ... && is_random_accessible_container<RemoveCVRef_t<Types>>::value );
}

template<class... TaggedTypes>
struct TaggedTuple
{
    static_assert( unique_types_v<typename TaggedTypes::TagType...>, "Type Conflicts!" );

    using this_type = TaggedTuple<TaggedTypes...>;
    using TagTuple = std::tuple<typename TaggedTypes::TagType...>;
    using ValueTuple = std::tuple<typename TaggedTypes::ValueType...>;


    template<class... Tags>
    using SubTupleType = TaggedTuple<
            TaggedType<Tags, NthType_t<find_type_index<true, Tags, typename TaggedTypes::TagType...>(), typename TaggedTypes::ValueType...>>...>;

    template<class... Tags>
    using SubNoRefTupleType = TaggedTuple<TaggedType<Tags,
                                                     NthType_t<find_type_index<true, Tags, typename TaggedTypes::TagType...>(),
                                                               std::remove_reference_t<typename TaggedTypes::ValueType>...>>...>;
    template<class... Tags>
    using SubRefTupleType = TaggedTuple<
            TaggedType<Tags, NthType_t<find_type_index<true, Tags, typename TaggedTypes::TagType...>(), typename TaggedTypes::ValueType &...>>...>;

    template<class... Tags>
    using SubConstRefTupleType = TaggedTuple<
            TaggedType<Tags,
                       NthType_t<find_type_index<true, Tags, typename TaggedTypes::TagType...>(), const typename TaggedTypes::ValueType &...>>...>;


    TaggedTuple() = default;

    template<class... ValueTypes>
    explicit TaggedTuple( ValueTypes &&... values ) : tup_{std::forward<ValueTypes>( values )...}
    {
    }

    auto &get_tuple()
    {
        return tup_;
    }
    const auto &get_tuple() const
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
    auto &&get() &&
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

    template<size_t idx>
    auto &&at() &&
    {
        return std::get<idx>( tup_ );
    }

    template<size_t i>
    auto tagged_value_at() const
    {
        using TagType = NthType_t<i, typename TaggedTypes::TagType...>;
        using ValueType = NthType_t<i, typename TaggedTypes::ValueType...>;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    template<size_t i>
    auto clone_tagged_value_at() const
    {
        using TagType = NthType_t<i, typename TaggedTypes::TagType...>;
        using ValueType = std::remove_reference_t<NthType_t<i, typename TaggedTypes::ValueType...>>;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    template<size_t i>
    auto ref_tagged_value_at()
    {
        using TagType = NthType_t<i, typename TaggedTypes::TagType...>;
        using ValueType = std::remove_reference_t<decltype( std::get<i>( std::declval<ValueTuple>() ) )> &;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    template<size_t i>
    auto ref_tagged_value_at() const
    {
        using TagType = NthType_t<i, typename TaggedTypes::TagType...>;
        using ValueType = const std::remove_reference_t<decltype( std::get<i>( std::declval<ValueTuple>() ) )> &;
        return TaggedValue<TagType, ValueType>{this->at<i>()};
    }

    /// @brief Shaddow copy subset of TaggedTuple. If ValueType is referencce type, returned ValueType is still reference type.
    template<class... Tags>
    auto copy( Tags... ) const
    {
        using ReturnType = SubTupleType<Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Clone (deep copy) subset of TaggedTuple. If ValueType is referencce type, returned ValueType is the referenced type.
    template<class... Tags>
    auto clone( Tags... ) const
    {
        using ReturnType = SubNoRefTupleType<Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Reference to const subset of TaggedTuple.
    template<class... Tags>
    auto ref( Tags... ) const
    {
        return get_ref_tuple<Tags...>();
    }

    /// @brief Reference to subset of TaggedTuple.
    template<class... Tags>
    auto ref( Tags... )
    {
        return get_ref_tuple<Tags...>();
    }

    /// @brief Reference to the whole TaggedTuple
    auto ref()
    {
        return get_ref_tuple<typename TaggedTypes::TagType...>();
    }

    auto ref() const
    {
        return get_ref_tuple<typename TaggedTypes::TagType...>();
    }

    /// @brief Join with another TaggedTuple
    /// @pre No TagType conflicts.
    /// @param checkAlignmentForDataFrame Throws std::runtime_error when resulting in non-aligned dataframe or non dataframe.
    template<bool checkAlignmentForDataFrame = true, class... TaggedTypes1>
    auto join( const TaggedTuple<TaggedTypes1...> &another ) const
    {
        static_assert( unique_types_v<typename TaggedTypes::TagType..., typename TaggedTypes1::TagType...>, "Type Conflicts!" );

        if constexpr ( checkAlignmentForDataFrame )
        {
            static_assert( !( is_dataframe && !another.is_dataframe ), "join dataframe with non-dataframe!" );
            static_assert( !( !is_dataframe && another.is_dataframe ), "join non-dataframe with dataframe!" );

            if constexpr ( is_dataframe && another.is_dataframe )
            {
                if ( !aligned() || !another.aligned() || aligned_size() != another.aligned_size() )
                {
                    assert( false && "join dataframes with different sizes!" );
                    throw std::runtime_error( "join dataframes with different sizes!" );
                }
            }
        }
        using ReturnType = TaggedTuple<TaggedTypes..., TaggedTypes1...>;
        return ReturnType{std::tuple_cat( tup_, another.get_tuple() )};
    }

    template<class... TaggedTypes1>
    auto operator+( const TaggedTuple<TaggedTypes1...> &another ) const
    {
        return join( another );
    }

    /// @brief For each element, call Function func(Tag, Value)
    template<class F, class... Tags>
    void for_each( const F &func, Tags... )
    {
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            for_each_( func, std::make_index_sequence<sizeof...( TaggedTypes )>(), tup_ );
        }
        else
        {
            for_each_( func, std::index_sequence<find_type_index<Tags, TaggedTypes...>()...>{}, tup_ );
        }
    }

    template<class F, class... Tags>
    void for_each( const F &func, Tags... ) const
    {
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            for_each_( func, std::make_index_sequence<sizeof...( TaggedTypes )>(), tup_ );
        }
        else
        {
            for_each_( func, std::index_sequence<find_type_index<Tags, TaggedTypes...>()...>{}, tup_ );
        }
    }

    /// @brief For each element, call Function func(Tag, Value, arg)
    template<size_t idx = 0, class F, class Arg1, class... Args>
    inline void for_each_arg( const F &func, Arg1 &&arg1, Args &&... args )
    {
        func( NthType_t<idx, typename TaggedTypes::TagType...>{}, at<idx>(), std::forward<Arg1>( arg1 ) );

        if constexpr ( sizeof...( args ) != 0 )
        {
            for_each_arg<idx + 1>( func, std::forward<Args>( args )... );
        }
    }

    template<class Stream>
    Stream &print_json( Stream &os ) const
    {
        os << "{";
        for_each( [&os]( auto tag, const auto &value ) {
            if constexpr ( !std::is_same_v<decltype( tag ), NthType_t<0, typename TaggedTypes::TagType...>> )
            {
                os << ",";
            }
            os << decltype( tag )::c_str() << ":" << value;
        } );
        os << "}";
        return os;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //                       DataFrame APIs
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////

    static constexpr bool is_dataframe = can_construct_dataframe<RemoveCVRef_t<typename TaggedTypes::ValueType>...>();

    struct df
    {
        template<class Tag>
        using RowElemType = NthType_t<find_type_index<true, Tag, typename TaggedTypes::TagType...>(),
                                      typename RemoveCVRef_t<typename TaggedTypes::ValueType>::value_type...>;

        template<class... Tags>
        using RowRefType = TaggedTuple<TaggedType<Tags, RefValueType_t<ValueRefType::Ref, RowElemType<Tags>>>...>;

        template<class... Tags>
        using RowType = TaggedTuple<TaggedType<Tags, RefValueType_t<ValueRefType::NoRef, RowElemType<Tags>>>...>;

        template<class... Tags>
        using RowConstRefType = TaggedTuple<TaggedType<Tags, RefValueType_t<ValueRefType::ConstRef, RowElemType<Tags>>>...>;

        template<class TaggedTupleT>
        struct RowView
        {
            static constexpr bool IsConst = std::is_const_v<TaggedTupleT>;

            TaggedTupleT *dataframe_ = nullptr;
            size_t idx_{0}; // row index

            template<class Tag>
            auto &get()
            {
                return dataframe_->template get<Tag>()[idx_];
            }

            template<class Tag>
            auto &operator[]( Tag )
            {
                return get<Tag>();
            }

            RowView &operator++()
            {
                ++idx_;
                return *this;
            }

            RowView &operator++( int )
            {
                RowView res{dataframe_, idx_};
                ++idx_;
                return res;
            }

            template<class... Tags>
            auto rowref( Tags... tags ) const
            {
                return dataframe_->rowref( idx_, tags... );
            }

            template<class... Tags>
            auto copy( Tags... tags ) const
            {
                return dataframe_->rowcopy( idx_, tags... );
            }

            bool is_end() const
            {
                return idx_ == dataframe_->size();
            }
        };
    };

    template<bool bDataFrame, class TaggedTupleT>
    struct Iterator
    {
        using value_type = typename df::template RowView<TaggedTupleT>;
        static constexpr bool IsConst = std::is_const_v<TaggedTupleT>;

        Iterator( TaggedTupleT *dataframe = nullptr, size_t idx = 0 ) : value_{dataframe, idx}
        {
        }

        value_type &operator*()
        {
            return value_;
        }

        value_type *operator->()
        {
            return &value_;
        }

        Iterator &operator++()
        {
            ++value_;
            return *this;
        }
        Iterator operator++( int )
        {
            Iterator res = *this;

            ++value_;
            return res;
        }

    protected:
        value_type value_;
    };

    template<class TaggedTupleT>
    struct Iterator<false, TaggedTupleT>
    {
        Iterator( TaggedTupleT *, size_t )
        {
        }
    };

    using iterator = Iterator<is_dataframe, this_type>;
    using const_iterator = Iterator<is_dataframe, const this_type>;

    /// @brief Get the min/aligned size of columns.
    template<class... Tags>
    size_t aligned_size( Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            return min_value( get<typename TaggedTypes::TagType>().size()... );
        }
        else
        {
            return min_value( get<Tags>().size()... );
        }
    }

    /// @brief Get the max size of columns.
    template<class... Tags>
    size_t max_size( Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            return max_value( get<typename TaggedTypes::TagType>().size()... );
        }
        else
        {
            return max_value( get<Tags>().size()... );
        }
    }

    /// @brief Get the size of column 0, which is the aligned dataframe row size. It's a fast way to get size, but may cause problem if it's not
    /// aligned.
    auto size() const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        return this->at<0>().size();
    }

    auto begin()
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        return iterator( this, 0 );
    }

    auto begin() const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        return const_iterator( this, 0 );
    }

    auto end()
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        return iterator( this, size() );
    }
    auto end() const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        return const_iterator( this, size() );
    }

    /// @brief check if all columns have the same size (aligned).
    bool aligned() const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        const auto N = size();
        return ( true && ... && ( N == get<typename TaggedTypes::TagType>().size() ) );
    }


    template<class... ValueType>
    this_type &append_row( ValueType &&... values )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for_each_arg( []( auto, auto &&value, auto &&arg ) { value.emplace( value.end(), std::forward<decltype( arg )>( arg ) ); },
                      std::forward<ValueType>( values )... );
        return *this;
    }

    template<class... Values>
    void append_row( const std::tuple<Values...> &tup )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for_each( [&]( auto tag, auto &col ) {
            constexpr size_t idx = find_type_index<true, decltype( tag ), typename TaggedTypes::TagType...>();
            col.emplace( col.end(), std::get<idx>( tup ) );
        } );
    }

    void append_row( std::tuple<typename TaggedTypes::ValueType...> &&tup )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for_each( [&]( auto tag, auto &col ) {
            constexpr size_t idx = find_type_index<true, decltype( tag ), typename TaggedTypes::TagType...>();
            col.emplace( col.end(), std::move( std::get<idx>( tup ) ) );
        } );
    }

    /// @brief For each row call func( TagValue&&... )
    template<class F, class... Tags>
    void for_each_row( F &&func, Tags... )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            if constexpr ( sizeof...( Tags ) == 0 )
                func( this->template get<typename TaggedTypes::TagType>()[i]... );
            else
                func( this->template get<Tags>()[i]... );
        }
    }
    template<class F, class... Tags>
    void for_each_row( F &&func, Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            if constexpr ( sizeof...( Tags ) == 0 )
                func( this->template get<typename TaggedTypes::TagType>()[i]... );
            else
                func( this->template get<Tags>()[i]... );
        }
    }

    /// @brief For each row call func( size_t idx, TagValue&&... )
    template<class F, class... Tags>
    void enumerate_each_row( F &&func, Tags... )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            if constexpr ( sizeof...( Tags ) == 0 )
                func( i, this->template get<typename TaggedTypes::TagType>()[i]... );
            else
                func( i, this->template get<Tags>()[i]... );
        }
    }
    template<class F, class... Tags>
    void enumerate_each_row( F &&func, Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            for ( size_t i = 0, N = size(); i < N; ++i )
            {
                if constexpr ( sizeof...( Tags ) == 0 )
                    func( i, this->template get<typename TaggedTypes::TagType>()[i]... );
                else
                    func( i, this->template get<Tags>()[i]... );
            }
        }
    }

    /// @brief Get TaggedTuple<Tag, Value&> at row idx.
    template<class... Tags>
    auto rowref( size_t idx, Tags... )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            return typename df::template RowRefType<typename TaggedTypes::TagType...>( this->template get<typename TaggedTypes::TagType>()[idx]... );
        }
        else
            return df::template RowRefType<Tags...>( this->template get<Tags>()[idx]... );
    }
    template<class... Tags>
    auto rowref( size_t idx, Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        if constexpr ( sizeof...( Tags ) == 0 )
        {
            return typename df::template RowConstRefType<typename TaggedTypes::TagType...>(
                    this->template get<typename TaggedTypes::TagType>()[idx]... );
        }
        else
            return typename df::template RowConstRefType<Tags...>( this->template get<Tags>()[idx]... );
    }

    template<class... Tags>
    auto rowcopy( size_t idx, Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        if constexpr ( sizeof...( Tags ) == 0 )
            return typename df::template RowType<typename TaggedTypes::TagType...>( this->template get<typename TaggedTypes::TagType>()[idx]... );
        else
            return typename df::template RowType<Tags...>( this->template get<Tags>()[idx]... );
    }

    /// @brief Stack another dataframe or non-datafame TaggedTuple. Another must have all TagType in current DataFrame.
    template<class... TaggedTypes1>
    this_type &stack( const TaggedTuple<TaggedTypes1...> &another )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );

        if constexpr ( another.is_dataframe )
        {
            for_each( [&]( auto tag, auto &value ) {
                using Tag = decltype( tag );
                auto &vec = another.template get<Tag>();
                value.insert( value.end(), vec.begin(), vec.end() );
            } );
        }
        else
        {
            for_each( [&]( auto tag, auto &value ) {
                using Tag = decltype( tag );
                value.emplace_back( another.template get<Tag>() );
            } );
        }
        return *this;
    }

    template<class... TaggedTypes1>
    this_type &stack( TaggedTuple<TaggedTypes1...> &&another )
    {
        static_assert( is_dataframe, "Not a DataFrame!" );

        if constexpr ( another.is_dataframe )
        {
            for_each( [&]( auto tag, auto &value ) {
                using Tag = decltype( tag );
                auto &vec = another.template get<Tag>();
                value.insert( value.end(), vec.begin(), vec.end() );
            } );
        }
        else
        {
            for_each( [&]( auto tag, auto &value ) {
                using Tag = decltype( tag );
                value.emplace_back( std::move( another.template get<Tag>() ) );
            } );
        }
        return *this;
    }

    template<class TaggedTupleT, class... Tags>
    auto stack_copy( const TaggedTupleT &another, Tags... ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );

        if constexpr ( sizeof...( Tags ) == 0 )
        {
            using ReturnType = this_type;
            ReturnType res{};
            res.stack( *this );
            return res.stack( another );
        }
        else
        {
            using ReturnType = SubTupleType<Tags...>;
            ReturnType res{};
            res.stack( *this );
            return res.stack( another );
        }
    }

    template<class TaggedTupleT, class... Tags>
    auto stack( const TaggedTupleT &another, Tags... ) const
    {
        return stack_copy( another, Tags{}... );
    }

    std::ostream &print_json_by_row( std::ostream &os ) const
    {
        static_assert( is_dataframe, "Not a DataFrame!" );
        os << "[";

        for ( size_t i = 0; i < size(); ++i )
        {
            if ( i > 0 )
            {
                os << ",";
            }
            os << rowref( i );
        }
        os << "]";
        return os;
    }

protected:
    template<class F, std::size_t... Is, class Tuple>
    static void for_each_( const F &func, std::index_sequence<Is...>, Tuple &&tup )
    {
        using expander = int[];
        (void)expander{0, ( (void)func( NthType_t<Is, typename TaggedTypes::TagType...>{}, std::get<Is>( tup ) ), 0 )...};
    }

    template<class... Tags>
    auto get_ref_tuple()
    {
        using ReturnType = SubRefTupleType<Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    template<class... Tags>
    auto get_ref_tuple() const
    {
        using ReturnType = SubConstRefTupleType<Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    ValueTuple tup_;
};

template<class... TaggedTypes>
using DataFrame = TaggedTuple<TaggedType<typename TaggedTypes::TagType, std::vector<RemoveCVRef_t<typename TaggedTypes::ValueType>>>...>;


template<class T, class... TaggedTypes>
std::ostream &operator<<( std::ostream &os, const TaggedTuple<T, TaggedTypes...> &tup )
{
    if constexpr ( TaggedTuple<T, TaggedTypes...>::is_dataframe )
    {
        return tup.print_json_by_row( os );
    }
    else
    {
        return tup.print_json( os );
    }
    //    os << "{";
    //    tup.for_each( [&os]( auto tag, const auto &value ) {
    //        if constexpr ( !std::is_same_v<decltype( tag ), typename T::TagType> )
    //        {
    //            os << ",";
    //        }
    //        os << decltype( tag )::c_str() << ":" << value;
    //    } );
    //    os << "}";
    //    return os;
}

} // namespace ftl
