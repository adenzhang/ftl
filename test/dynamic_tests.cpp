#include <ftl/dynamic.h>
#include <ftl/ftl.h>
#include <any>
using namespace std;
using namespace ftl;

namespace schema
{
using DynMap = dynamic_map<std::string, std::string>;
using DynVec = dynamic_vec<std::string, std::string>;
using DynType = dynamic_var<std::string, std::string>;

FTL_CHECK_EXPR( has_global_from_chars, std::from_chars( ::std::declval<const char *>(), ::std::declval<const char *>(), ::std::declval<T>() ) );
FTL_CHECK_EXPR( has_from_chars, ::std::declval<T>().from_chars( ::std::declval<const char *>(), ::std::declval<const char *>() ) );

template<class T, class = std::enable_if_t<has_global_from_chars<T>::value, void>>
struct object_read
{
    std::optional<T> operator()( std::string_view s ) const
    {
        T v;
        auto [p, err] = std::from_chars( s.data(), s.data() + s.length(), v );
        if ( err == std::errc() )
        {
            return {v};
        }
        return {};
    }
};
} // namespace schema


/////////////////////////////////////////////////////////////////////////////////////////////
/// dynamic
///   container of pointer to dynamic_var
/// ---------------------------------
///  container of dyn_var
///  dyn_var of pointer to container
/////////////////////////////////////////////////////////////////////////////////////////////

namespace dyn
{


/// @tparam K Key type of dynamic type.
/// @tparam PrimitiveTypes..., all the primitive types.

struct dyn_recursive_var
{
    virtual ~dyn_recursive_var() = default;
};

template<class K, class... PrimitiveTypes>
struct dyn_vec;

template<class K, class... PrimitiveTypes>
struct dyn_map;

template<class K, class... PrimitiveTypes>
using dyn_map_ptr = std::unique_ptr<dyn_map<K, PrimitiveTypes...>>;

template<class K, class... PrimitiveTypes>
using dyn_vec_ptr = std::unique_ptr<dyn_map<K, PrimitiveTypes...>>;

template<class K, class... PrimitiveTypes>
using dyn_var = std::variant<std::nullptr_t, PrimitiveTypes..., dyn_vec_ptr<K, PrimitiveTypes...>, dyn_map_ptr<K, PrimitiveTypes...>>;

template<class K, class... PrimitiveTypes>
struct dyn_vec : public std::vector<dyn_var<K, PrimitiveTypes...>>
{
    using base_type = std::vector<dyn_var<K, PrimitiveTypes...>>;
    base_type &as_base()
    {
        return *this;
    }
};

template<class K, class... PrimitiveTypes>
struct dyn_map : public std::unordered_map<K, dyn_var<K, PrimitiveTypes...>>
{
    using base_type = std::unordered_map<K, dyn_var<K, PrimitiveTypes...>>;
    base_type as_base()
    {
        return *this;
    }
};


template<class K, class... PrimitiveTypes>
struct dyn_types
{
    using var_type = dyn_var<K, PrimitiveTypes...>;
    using map_type = dyn_map<K, PrimitiveTypes...>;
    using vec_type = dyn_vec<K, PrimitiveTypes...>;

    using map_ptr_type = dyn_map_ptr<K, PrimitiveTypes...>;
    using vec_ptr_type = dyn_vec_ptr<K, PrimitiveTypes...>;
};

/// @tparam DynT dyn_types type
/// @tparam ET Element type: dyn_map, dyn_vec, or primitive type.
/// return dyn_var
template<class DynTypes, class... Args>
auto dyn_make_vec( Args &&... args )
{
    using var_t = typename DynTypes::var_type;
    return var_t( std::in_place_type<typename DynTypes::vec_ptr_type>,
                  std::make_unique<typename DynTypes::vec_type>( std::forward<Args>( args )... ) );
}

template<class DynTypes, class ET, class... Args>
auto dyn_make_map( Args &&... args )
{
    using var_t = typename DynTypes::var_type;
    return var_t( std::in_place_type<typename DynTypes::map_ptr_type>,
                  std::make_unique<typename DynTypes::vec_type>( std::forward<Args>( args )... ) );
}

template<class DynTypes, class ET, class... Args>
auto dyn_make_dyn( Args &&... args )
{
    using var_t = typename DynTypes::var_type;
    return var_t( std::in_place_type<ET>, std::forward<Args>( args )... );
}


} // namespace dyn

// template<class K, class... PrimitiveTypes>
// using dyn_map_internal = std::unordered_map<K, dyn_var<K, PrimitiveTypes...>>;

// template<class K, class... PrimitiveTypes>
// using dyn_vec_internal = std::vector<K, dyn_var<K, PrimitiveTypes...>>;

// template<class K, class... PrimitiveTypes>
// void dyn_vec<K, PrimitiveTypes...>::destroy()
//{
//    using Vec = dyn_vec_internal<K, PrimitiveTypes...>;
//    reinterpret_cast<Vec>( this->pDyn )->~Vec();
//}

ADD_TEST_CASE( dynamic_tests )
{
    {
        using D = dyn::dyn_var<int, int>;
        D d;
    }
    {
        using namespace schema;

        auto m = make_dynamic<DynType, DynMap>();
        auto a = make_dynamic<DynType, DynVec>();

        static constexpr size_t has_string = variant_accepted_index<std::string, DynType>::value;
        static constexpr size_t has_int = variant_accepted_index<int, DynType>::value;

        size_t npos = variant_npos;
        m.map_add<std::string>( "n1", "abd" );
        a.vec_add<std::string>( "abd" );

        std::cout << a << std::endl;

        auto ta = a.clone();
        m.map_add<DynType>( "anarray", std::move( ta ) );

        std::cout << m << std::endl;

        m.map_add<DynMap>( "submap" ).map_add<std::string>( "k1", "v1" );
        std::cout << m << std::endl;
    }
}
