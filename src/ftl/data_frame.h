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

#include <ftl/tagged_tuple.h>
#include <ftl/container_serialization.h>
#include <vector>

using namespace ftl::serialization;
namespace ftl::df
{

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

template<ValueRefType refType, class TaggedTypeT>
using RefTaggedType_t = TaggedType<typename TaggedTypeT::TagType, RefValueType_t<refType, typename TaggedTypeT::ValueType>>;

template<template<ValueRefType, typename...> class TupleT, class... TaggedTypes>
struct AlterTaggedTuple
{

    template<ValueRefType refType, typename... Args>
    using ThisTupleTypeT = TupleT<refType, Args...>;

    template<ValueRefType refType>
    using ThisTupleType = TupleT<refType, TaggedTypes...>;

    template<ValueRefType refType, class... Tags>
    using SubType = TupleT<
            refType,
            TaggedType<Tags, NthType_t<find_type_index<true, Tags, typename TaggedTypes::TagType...>(), typename TaggedTypes::ValueType...>>...>;

    template<class... TaggedTypes1>
    using ConcatType = TupleT<ValueRefType::AsIs, TaggedTypes..., TaggedTypes1...>;
};

template<class AlterTaggedTupleT, class... TaggedTypes>
struct TaggedTupleBase
{
    static_assert( unique_types_v<typename TaggedTypes::TagType...>, "Type Conflicts!" );

    using TagTuple = std::tuple<typename TaggedTypes::TagType...>;
    using ValueTuple = std::tuple<typename TaggedTypes::ValueType...>;

    TaggedTupleBase() = default;

    template<class... ValueTypes>
    explicit TaggedTupleBase( ValueTypes &&... values ) : tup_{std::forward<ValueTypes>( values )...}
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
        using ReturnType = typename AlterTaggedTupleT::template SubType<ValueRefType::AsIs, Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    /// @brief Clone (deep copy) subset of TaggedTuple. If ValueType is referencce type, returned ValueType is the referenced type.
    template<class... Tags>
    auto clone( Tags... ) const
    {
        using ReturnType = typename AlterTaggedTupleT::template SubType<ValueRefType::NoRef, Tags...>;

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

    /// @brief Concat with another TaggedTuple.
    /// @note If ThisTupleTypeT is data frame, it is always copied.
    /// @pre No TagType conflicts. ChildSubTuple must be the same.
    template<ValueRefType refType, class... TaggedTypes1>
    auto concat( const typename AlterTaggedTupleT::template ThisTupleTypeT<refType, TaggedTypes1...> &another ) const
    {
        static_assert( unique_types_v<typename TaggedTypes::TagType..., typename TaggedTypes1::TagType...>, "Type Conflicts!" );

        using ReturnType = typename AlterTaggedTupleT::template ConcatType<TaggedTypes1...>;
        return ReturnType{std::tuple_cat( tup_, another.get_value_tuple() )};
    }

    template<ValueRefType refType, class... TaggedTypes1>
    auto operator+( const typename AlterTaggedTupleT::template ThisTupleTypeT<refType, TaggedTypes1...> &another ) const
    {
        return concat( another );
    }

    /// @brief For each element, call Function func(Tag, Value)
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

private:
    template<class F, std::size_t... Is, class Tuple>
    static void for_each_( const F &func, std::index_sequence<Is...>, Tuple &&tup )
    {
        using expander = int[];
        (void)expander{0, ( (void)func( NthType_t<Is, typename TaggedTypes::TagType...>{}, std::get<Is>( tup ) ), 0 )...};
    }

    template<class... Tags>
    auto get_ref_tuple()
    {
        using ReturnType = typename AlterTaggedTupleT::template SubType<ValueRefType::Ref, Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    template<class... Tags>
    auto get_ref_tuple() const
    {
        using ReturnType = typename AlterTaggedTupleT::template SubType<ValueRefType::ConstRef, Tags...>;

        return ReturnType{at<find_type_index<true, Tags, typename TaggedTypes::TagType...>()>()...};
    }

    ValueTuple tup_;
};

template<ValueRefType refType,
         template<ValueRefType, typename...>
         class ChildTuple,
         template<ValueRefType, typename>
         class ToBaseTupleValueT,
         class... TaggedTypes>
using TupleStorage = TaggedTupleBase<AlterTaggedTuple<ChildTuple, TaggedTypes...>,
                                     TaggedType<typename TaggedTypes::TagType, ToBaseTupleValueT<refType, typename TaggedTypes::ValueType>>...>;


template<ValueRefType refType, class ValueT>
using TaggedTupleToBaseTupleValue = RefValueType_t<refType, ValueT>;

template<ValueRefType refType, class... TaggedTypes>
struct TaggedTuple : TupleStorage<refType, TaggedTuple, TaggedTupleToBaseTupleValue, TaggedTypes...>
{

    template<ValueRefType refTypeT, class ValueT>
    using ToBaseTupleValueT = TaggedTupleToBaseTupleValue<refTypeT, ValueT>;


    using base_type = TupleStorage<refType, TaggedTuple, TaggedTupleToBaseTupleValue, TaggedTypes...>;

    template<class... ValueTypes>
    explicit TaggedTuple( ValueTypes &&... values ) : base_type{std::forward<ValueTypes>( values )...}
    {
    }
};

template<class... TaggedTypes>
auto make_tagged_tuple( TaggedTupleToBaseTupleValue<ValueRefType::NoRef, typename TaggedTypes::ValueType> &&... values )
{
    return TaggedTuple<ValueRefType::NoRef, TaggedTypes...>( std::forward<decltype( values )>( values )... );
}


template<ValueRefType refTyppe, class T, class... TaggedTypes>
std::ostream &operator<<( std::ostream &os, const TaggedTuple<refTyppe, T, TaggedTypes...> &tup )
{
    os << "{";
    tup.for_each( [&os]( auto tag, const auto &value ) {
        if constexpr ( !std::is_same_v<decltype( tag ), typename T::TagType> )
        {
            os << ",";
        }
        os << decltype( tag )::c_str() << ":" << value;
    } );
    os << "}";
    return os;
}

template<class DataFrame>
struct DataFrameIterator
{
    using value_type = std::conditional_t<std::is_same_v<DataFrame, std::remove_const_t<DataFrame>>,
                                          typename DataFrame::RowRefType,
                                          typename DataFrame::ConstRowRefType>;

    using this_type = DataFrameIterator;

    DataFrameIterator( DataFrame *container = nullptr, size_t idx = 0 ) : container_( container ), idx_( idx )
    {
    }

    DataFrameIterator( const this_type & ) = default;
    DataFrameIterator &operator=( const this_type & ) = default;

    bool operator==( const this_type &another ) const
    {
        return container_ == another.container_ && idx_ == another.idx_;
    }

    bool operator!=( const this_type &another ) const
    {
        return !operator==( another );
    }

    this_type &operator++()
    {
        ++idx_;
        return *this;
    }

    this_type operator++( int )
    {
        this_type res( container_, idx_ );
        ++idx_;
        return res;
    }

    this_type &operator--()
    {
        --idx_;
        return *this;
    }

    this_type operator--( int )
    {
        this_type res( container_, idx_ );
        --idx_;
        return res;
    }

    value_type &operator*()
    {
        get_value();
        return *value_;
    }

    value_type *operator->()
    {
        get_value();
        return &( *value_ );
    }

protected:
    void get_value()
    {
        if ( !value_ )
            value_.emplace( container_->rowref( idx_ ) );
    }
    DataFrame *container_{nullptr};
    size_t idx_{0};
    std::optional<value_type> value_;
};



template<ValueRefType refType, class ValueT>
using ColDataFrameToBaseTupleValue = RefValueType_t<refType, std::vector<ValueT>>;

template<ValueRefType refType, class... TaggedTypes>
class ColDataFrame : public TupleStorage<refType, ColDataFrame, ColDataFrameToBaseTupleValue, TaggedTypes...>
{
public:
    /// @note Child type must define ToBaseTupleValueT.
    template<ValueRefType refTypeT, class ValueT>
    using ToBaseTupleValueT = ColDataFrameToBaseTupleValue<refTypeT, ValueT>;

    using this_type = ColDataFrame;
    using Storage = TupleStorage<refType, ColDataFrame, ColDataFrameToBaseTupleValue, TaggedTypes...>;

    using Storage::at;
    using Storage::clone;
    using Storage::clone_tagged_value_at;
    using Storage::copy;
    using Storage::for_each;
    using Storage::for_each_arg;
    using Storage::get;
    using Storage::ref;
    using Storage::ref_tagged_value_at;
    using Storage::tagged_value_at;

    using Storage::concat; ///< @note concat another with diferent size, which is allowed but may cause runtime exception.

    using RowRefType = TaggedTuple<ValueRefType::Ref, TaggedTypes...>;
    using RowType = TaggedTuple<ValueRefType::NoRef, TaggedTypes...>;
    using ConstRowRefType = TaggedTuple<ValueRefType::ConstRef, TaggedTypes...>;

    using iterator = DataFrameIterator<ColDataFrame>;
    using const_iterator = DataFrameIterator<const ColDataFrame>;

    ColDataFrame() = default;
    ColDataFrame( ColDataFrame && ) = default;
    ColDataFrame( const ColDataFrame & ) = default;

    template<class... ValueTypes>
    explicit ColDataFrame( ValueTypes &&... values ) : Storage{std::forward<ValueTypes>( values )...}
    {
    }

    void append_row( typename TaggedTypes::ValueType &&... values )
    {
        for_each_arg( []( auto, auto &&value, auto &&arg ) { value.insert( value.end(), std::forward<decltype( arg )>( arg ) ); },
                      std::forward<typename TaggedTypes::ValueType>( values )... );
    }

    void append_row( const std::tuple<typename TaggedTypes::ValueType...> &tup )
    {
        for_each( [&]( auto tag, auto &col ) {
            constexpr size_t idx = find_type_index<true, decltype( tag ), typename TaggedTypes::TagType...>();
            col.insert( col.end(), std::get<idx>( tup ) );
        } );
    }

    void append_row( std::tuple<typename TaggedTypes::ValueType...> &&tup )
    {
        for_each( [&]( auto tag, auto &col ) {
            constexpr size_t idx = find_type_index<true, decltype( tag ), typename TaggedTypes::TagType...>();
            col.insert( col.end(), std::move( std::get<idx>( tup ) ) );
        } );
    }

    size_t size() const
    {
        return this->template at<0>().size();
    }

    /// @brief For each row call func( TaggedValue<Tag, Value&>... )
    template<class F>
    void for_each_row( F &&func )
    {
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            func( TaggedValue<typename TaggedTypes::TagType, typename TaggedTypes::ValueTypee &>{
                    this->template get<typename TaggedTypes::TagType>()}... );
        }
    }

    /// @brief For each row call func( size_t idx, TaggedValue<Tag, Value&>... )
    template<class F>
    void enumerate_each_row( F &&func )
    {
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            func( i,
                  TaggedValue<typename TaggedTypes::TagType, typename TaggedTypes::ValueTypee &>{
                          this->template get<typename TaggedTypes::TagType>()[i]}... );
        }
    }
    template<class F>
    void enumerate_each_row( F &&func ) const
    {
        for ( size_t i = 0, N = size(); i < N; ++i )
        {
            func( i,
                  TaggedValue<typename TaggedTypes::TagType, const typename TaggedTypes::ValueTypee &>{
                          this->template get<typename TaggedTypes::TagType>()[i]}... );
        }
    }

    /// @brief Get TaggedTuple<Tag, Value&> at row idx.
    RowRefType rowref( size_t idx )
    {
        return RowRefType{this->template get<typename TaggedTypes::TagType>()[idx]...};
    }

    ConstRowRefType rowref( size_t idx ) const
    {
        return ConstRowRefType{this->template get<typename TaggedTypes::TagType>()[idx]...};
    }

    RowType rowcopy( size_t idx ) const
    {
        return RowType{this->template get<typename TaggedTypes::TagType>()[idx]...};
    }

    //// rewrite base member functions


    ///////// Iterator

    auto begin()
    {
        return DataFrameIterator( this, 0 );
    }

    auto end()
    {
        return DataFrameIterator( this, size() );
    }

    auto begin() const
    {
        return DataFrameIterator( this, 0 );
    }
    auto end() const
    {
        return DataFrameIterator( this, size() );
    }
};

template<class... TaggedTypes>
auto make_dataframe( ColDataFrameToBaseTupleValue<ValueRefType::AsIs, typename TaggedTypes::ValueType> &&... values )
{
    return ColDataFrame<ValueRefType::AsIs, TaggedTypes...>( std::forward<decltype( values )>( values )... );
}
template<class... TaggedTypes>
auto make_dataframe()
{
    return ColDataFrame<ValueRefType::AsIs, TaggedTypes...>{};
}

template<ValueRefType refType, class... TaggedTypes>
std::ostream &operator<<( std::ostream &os, const ColDataFrame<refType, TaggedTypes...> &df )
{
    os << "[";

    for ( size_t i = 0; i < df.size(); ++i )
    {
        if ( i > 0 )
        {
            os << ",";
        }
        os << df.rowref( i );
    }
    os << "]";
    return os;
}

/////////////////////////////////////////////////////
///
///   DataFrameByCol
// template<class... TaggedTypes>
// using DataFrameByCol = TaggedTuple<TaggedType<typename TaggedTypes::TagType, std::vector<typename TaggedTypes::ValueType>>...>;

// template<class... TaggedTypes>
// void append_row( TaggedTuple<TaggedTypes...> &df, typename TaggedTypes::ValueType::value_type &&... values )
//{
//    df.for_each_arg( []( auto, auto &value, auto &&arg ) { value.insert( value.end(), std::forward<decltype( arg )>( arg ) ); },
//                     std::forward<typename TaggedTypes::ValueType::value_type>( values )... );
//}

// template<class... TaggedTypes>
// class RowDataFrame : std::vector<TaggedTuple<ValueRefType::NoRef, TaggedTypes...>>
//{
// public:
//    using RowType = TaggedTuple<TaggedTypes...>;

//    using Storage = std::vector<TaggedTuple<TaggedTypes...>>;
//};

// template<class T, class... TaggedTypes>
// std::ostream &operator<<( std::ostream &os, const RowDataFrame<T, TaggedTypes...> &df )
//{
//    os << "[";

//    for ( size_t i = 0; i < df.size(); ++i )
//    {
//        if ( i > 0 )
//        {
//            os << ",";
//        }
//        os << df[i];
//    }
//    os << "]";
//    return os;
//}
} // namespace ftl::df
