#pragma once
#include <functional>
#include <variant>

////============ FTL_CHECK_EXPR ================

// write an expression in terms of type 'T' to create a trait which is true for a type which is valid within the expression
#define FTL_CHECK_EXPR(traitsName, ...)                                               \
    template <typename U = void>                                                      \
    struct traitsName {                                                               \
        using type = traitsName;                                                      \
                                                                                      \
        template <class T>                                                            \
        static auto Check(int) -> decltype(__VA_ARGS__, bool());                      \
                                                                                      \
        template <class>                                                              \
        static int Check(...);                                                        \
                                                                                      \
        static const bool value = ::std::is_same<decltype(Check<U>(0)), bool>::value; \
    }

#define FTL_CHECK_EXPR2(traitsName, ...)                                                   \
    template <typename U1 = void, typename U2 = void>                                      \
    struct traitsName {                                                                    \
        using type = traitsName;                                                           \
                                                                                           \
        template <class T1, class T2>                                                      \
        static auto Check(int) -> decltype(__VA_ARGS__, bool());                           \
                                                                                           \
        template <class, class>                                                            \
        static int Check(...);                                                             \
                                                                                           \
        static const bool value = ::std::is_same<decltype(Check<U1, U2>(0)), bool>::value; \
    }

#define FTL_HAS_MEMBER(member, name) FTL_CHECK_EXPR(name, ::std::declval<T>().member)

#define FTL_IS_COMPATIBLE_FUNC_ARG(func, name) FTL_CHECK_EXPR(name, func(::std::declval<T>()))

#define FTL_IS_COMPATIBLE_FUNC_ARG_LVALUE(func, name) FTL_CHECK_EXPR(name, func(::std::declval<T&>()))

#define FTL_CHECK_EXPR_TYPE(traitsName, ...)                                                     \
    template <typename R, typename U = void>                                                     \
    struct traitsName {                                                                          \
        using type = traitsName;                                                                 \
                                                                                                 \
        template <class T>                                                                       \
        static std::enable_if_t<std::is_same<R, decltype(__VA_ARGS__)>::value, bool> Check(int); \
                                                                                                 \
        template <class>                                                                         \
        static int Check(...);                                                                   \
                                                                                                 \
        static const bool value = ::std::is_same<decltype(Check<U>(0)), bool>::value;            \
    }

#define FTL_HAS_MEMBER_TYPE(member, name) FTL_CHECK_EXPR_TYPE(name, ::std::declval<T>().member)

////============================

//=========== object attribute definition macros
#define DEF_ATTRI_GETTER(name, member) \
public:                                \
    const auto& get_##name() const     \
    {                                  \
        return member;                 \
    }

#define DEF_ATTRI_SETTER(name, member)         \
public:                                        \
    void set_##name(const decltype(member)& v) \
    {                                          \
        member = v;                            \
    }

#define DEF_ATTRI(name, type, member, defaultval) \
    type member = defaultval;                     \
                                                  \
public:                                           \
    const auto& get_##name() const                \
    {                                             \
        return member;                            \
    }                                             \
    void set_##name(const decltype(member)& v)    \
    {                                             \
        member = v;                               \
    }

#define DEF_ATTRI_ACCESSOR(name, member)       \
public:                                        \
    const auto& get_##name() const             \
    {                                          \
        return member;                         \
    }                                          \
    void set_##name(const decltype(member)& v) \
    {                                          \
        member = v;                            \
    }

namespace ftl {

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...)->overload<Ts...>;

//================= tuple helpers ========================
template <typename T, typename Tuple>
struct tuple_has_type;

template <typename T, typename... Us>
struct tuple_has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {
};

//=================== variant helpers ===================

template <typename _Tp, typename _Variant, typename = void>
struct variant_accepted_index {
    static constexpr size_t value = std::variant_npos;
};

template <typename _Tp, typename... _Types>
struct variant_accepted_index<
    _Tp, std::variant<_Types...>,
    decltype(std::__detail::__variant::__overload_set<_Types...>::_S_fun(std::declval<_Tp>()),
        std::declval<void>())> {
    static constexpr size_t value = sizeof...(_Types) - 1
        - decltype(std::__detail::__variant::__overload_set<_Types...>::
                  _S_fun(std::declval<_Tp>()))::value;
};
template <class T, class V>
struct variant_has_type {
    static constexpr bool value = variant_accepted_index<T, V>::value != std::variant_npos;
};
//== MemberGetter
template <class ObjT, class T>
struct MemberGetter_Var {
    T ObjT::*pMember;

    MemberGetter_Var(T ObjT::*pMember = nullptr)
        : pMember(pMember)
    {
    }

    const T& operator()(const ObjT& obj) const
    {
        return obj.*pMember;
    }
};

template <class ObjT, class T>
struct MemberGetter_Func {
    using Fn = const T& (ObjT::*)() const;
    Fn pMember;

    MemberGetter_Func(Fn pMember = nullptr)
        : pMember(pMember)
    {
    }

    const T& operator()(const ObjT& obj) const
    {
        return (obj.*pMember)();
    }
};

template <class ObjT, class T>
struct MemberGetter {
    using Fn = std::function<const T&(const ObjT&)>;
    Fn func;

    MemberGetter(Fn&& g)
        : func(std::move(g))
    {
    }

    const T& operator()(const ObjT& obj) const
    {
        return func(obj);
    }
};

//================= Member Setter/Getter ======================

template <class ObjT, class T>
struct MemberSetter_Var {
    T ObjT::*pMember;

    MemberSetter_Var(T ObjT::*pMember = nullptr)
        : pMember(pMember)
    {
    }

    void operator()(ObjT& obj, const T& v) const
    {
        obj.*pMember = v;
    }
};

template <class ObjT, class T>
struct MemberSetter_Func {
    using Fn = void (ObjT::*)(const T&);
    Fn pMember = nullptr;

    MemberSetter_Func(Fn pMember = nullptr)
        : pMember(pMember)
    {
    }

    void operator()(ObjT& obj, const T& v) const
    {
        (obj.*pMember)(v);
    }
};

template <class ObjT, class T>
struct MemberSetter {
    using Fn = std::function<void(ObjT&, const T&)>;
    Fn func;

    MemberSetter(Fn&& s)
        : func(std::move(s))
    {
    }

    void operator()(ObjT& obj, const T& v) const
    {
        return func(obj, v);
    }
};

template <class ObjT, class T>
struct Member {
    using MemGetFn = const T& (ObjT::*)() const;
    using MemSetFn = void (ObjT::*)(const T&);

    MemberGetter<ObjT, T> getter;
    MemberSetter<ObjT, T> setter;

    Member(T ObjT::*pMember = nullptr)
        : getter(MemberGetter_Var(pMember))
        , setter(MemberSetter_Var(pMember))
    {
    }

    Member(const MemGetFn g, const MemberSetter<ObjT, T> s)
        : getter(MemberGetter_Func(g))
        , setter(MemberSetter_Func(s))
    {
    }
    Member(const typename MemberGetter<ObjT, T>::Fn& g, const typename MemberSetter<ObjT, T>::Fn& s)
        : getter(g)
        , setter(s)
    {
    }

    const T& get(const ObjT& obj) const
    {
        return getter(obj);
    }

    T& get(ObjT& obj) const
    {
        return const_cast<T&>(getter(obj));
    }

    void set(ObjT& obj, const T& v) const
    {
        return setter(obj, v);
    }
};
}
