#include <ftl/dynamic.h>
#include <ftl/ftl.h>

using namespace std;
using namespace ftl;

namespace schema {
using DynMap = dynamic_map<std::string, std::string>;
using DynArray = dynamic_array<std::string, std::string>;
using DynType = dynamic_var<std::string, std::string>;

FTL_CHECK_EXPR(has_global_from_chars, std::from_chars(::std::declval<const char*>(), ::std::declval<const char*>(), ::std::declval<T>()));
FTL_CHECK_EXPR(has_from_chars, ::std::declval<T>().from_chars(::std::declval<const char*>(), ::std::declval<const char*>()));

template <class T, class = std::enable_if_t<has_global_from_chars<T>::value, void>>
struct object_read {
    std::optional<T> operator()(std::string_view s) const
    {
        T v;
        auto [p, err] = std::from_chars(s.data(), s.data() + s.length(), v);
        if (err == std::errc()) {
            return { v };
        }
        return {};
    }
};
}

//FTL_CHECK_EXPR2(variant_has_type, std::get<T2>(std::declval<T1>()));

TEST_FUNC(dynamic_tests)
{
    using namespace schema;

    auto m = make_dynamic<DynType, DynMap>();
    auto a = make_dynamic<DynType, DynArray>();

    static constexpr size_t has_string = variant_accepted_index<std::string, DynType>::value;
    static constexpr size_t has_int = variant_accepted_index<int, DynType>::value;

    size_t npos = variant_npos;
    add_dynamic_map<std::string>(m, "n1", "abd");
    add_dynamic_array<std::string>(a, "abd");

    auto ta = deep_copy(a);
    add_dynamic_map<DynType>(m, "anarray", std::move(ta));
}
