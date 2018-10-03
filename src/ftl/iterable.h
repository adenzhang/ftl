#pragma once
#include <array>
#include <vector>

namespace ftl {

template <class ContainerT, bool bIsRandomAccess = false, bool bContiguousMemory = false>
struct iterable;

template <class Iter>
struct iterable_base {
    auto& operator*() const
    {
        return static_cast<const Iter*>(this)->value();
    }

    auto operator-> () const
    {
        return &static_cast<const Iter*>(this)->value();
    }

    auto& operator++()
    {
        static_cast<Iter*>(this)->next();
        return static_cast<Iter*>(this)->value();
    }

    auto operator++(int)
    {
        return static_cast<Iter*>(this)->next();
    }

    operator bool() const
    {
        return static_cast<const Iter*>(this)->has_next();
    }

    bool operator==(const Iter& a) const
    {
        return &static_cast<const Iter*>(this)->value() == &a.value();
    }

    template <class F>
    const Iter& for_each(F&& f) const
    {
        for (auto it = static_cast<const Iter*>(this)->slice(); it; ++it) {
            f(*it);
        }
        return *static_cast<const Iter*>(this);
    }

    template <class F, class A>
    A reduce(F&& f, A&& a) const
    {
        A t = std::forward<A>(a);
        for (auto it = static_cast<const Iter*>(this)->slice(); it; ++it) {
            t = f(std::move(t), *it);
        }
        return std::move(t);
    }

    std::ptrdiff_t distance(const Iter& it)
    {
        if constexpr (Iter::is_cont) {
            return std::distance(&it.value(), &static_cast<const Iter*>(this)->value());
        } else {
            std::ptrdiff_t n = 0;
            for (auto a = static_cast<const Iter*>(this)->slice(); a != it; ++a, ++n)
                ;
            return n;
        }
    }
};

template <class ContainerT>
struct iterable<ContainerT, false, false> : public iterable_base<iterable<ContainerT, false, false>> {
    using value_type = std::remove_reference_t<decltype(*std::declval<ContainerT>().begin())>;
    using underlying_iter = decltype(std::declval<ContainerT>().begin());
    static constexpr bool is_ra = false;
    static constexpr bool is_cont = false;

    iterable(ContainerT& t)
        : it(t.begin())
        , itEnd(t.end())
    {
    }

    iterable(underlying_iter it0, underlying_iter it1)
        : it(it0)
        , itEnd(it1)
    {
    }

    value_type& value() const
    {
        return *(it);
    }

    value_type& next()
    {
        return *(it++);
    }

    bool has_next() const
    {
        return it != itEnd;
    }

    iterable slice() const
    {
        return iterable(it, itEnd);
    }

protected:
    underlying_iter it, itEnd;
};

template <class ContainerT>
struct iterable<ContainerT, true, false> : public iterable_base<iterable<ContainerT, true, false>> // random access
{
    using value_type = std::remove_reference_t<decltype(*std::declval<ContainerT>().begin())>;
    using underlying_iter = decltype(std::declval<ContainerT>().begin());

    static constexpr bool is_ra = true;
    static constexpr bool is_cont = false;

    iterable(ContainerT& t, size_t offset = 0)
        : mC(t)
        , mOffset(offset)
        , mSize(t.size())
    {
    }

    value_type& value() const
    {
        return at(mCurr);
    }

    value_type& at(size_t i) const
    {
        return mC.at(i + mOffset);
    }

    size_t size() const
    {
        return mSize;
    }

    value_type& operator[](size_t i)
    {
        return mC[i];
    }

    iterable slice(size_t i = 0) const
    {
        return iterable(mC, i);
    }

    //---
    value_type& next()
    {
        return at(mCurr++);
    }

    bool has_next() const
    {
        return mCurr < mSize;
    }

    void reset() // reset current position
    {
        mSize = 0;
    }

protected:
    ContainerT& mC;
    size_t mOffset = 0, mSize = 0, mCurr = 0;
};

template <class ContainerT>
struct iterable<ContainerT, true, true> : public iterable_base<iterable<ContainerT, true, true>> {
    using value_type = std::remove_reference_t<decltype(*std::declval<ContainerT>().begin())>;
    using underlying_iter = decltype(std::declval<ContainerT>().begin());
    static constexpr bool is_ra = true;
    static constexpr bool is_cont = false;

    iterable(value_type* v, size_t n)
        : mArr(v)
        , mSize(n)
    {
    }

    iterable(underlying_iter it0, underlying_iter it1)
        : mArr(&(*it0))
        , mSize(&(*it0))
    {
    }

    iterable(ContainerT& c)
        : mArr(&c.front())
        , mSize(c.size())
    {
    }

    value_type& value() const
    {
        return at(mCurr);
    }

    value_type& at(size_t i) const
    {
        return mArr[i];
    }

    size_t size() const
    {
        return mSize;
    }

    value_type& operator[](size_t i)
    {
        return at(i);
    }

    iterable slice(size_t i = 0) const
    {
        return iterable(&at(i), mSize - i);
    }

    //---
    value_type& next()
    {
        return at(mCurr++);
    }

    bool has_next() const
    {
        return mCurr < mSize;
    }

    void reset() // reset current position
    {
        mSize = 0;
    }

protected:
    value_type* mArr = nullptr;
    size_t mSize = 0, mCurr = 0;
};

struct MakeIterable {
    template <class Args>
    struct is_contiguous : public std::false_type {
    };

    template <class... Args>
    struct is_contiguous<std::vector<Args...>> : public std::true_type {
    };

    template <class... Args>
    struct is_contiguous<std::array<Args...>> : public std::true_type {
    };

    template <class T>
    struct is_contiguous<T*> : public std::true_type {
    };

    template <class C>
    struct is_random_access {
        template <class R, typename U = void>
        struct IsRandamAccessible {
            template <class T>
            //            static auto Check( int ) -> decltype( std::declval<T>()[0], bool() );
            static std::enable_if_t<std::is_same<R, decltype(std::declval<T>()[0])>::value, bool> Check(int);

            template <class>
            static int Check(...);

            static const bool value = ::std::is_same<decltype(Check<U>(0)), bool>::value;
        };

        // if T has member function value_type operator[](size_t)
        static constexpr auto value = IsRandamAccessible<decltype(*std::declval<C>().begin()), C>::value;
    };

    template <class T>
    auto operator()(T* v, size_t N) const
    {
        return iterable<std::vector<T>, true, true>(v, N);
    }

    template <class T>
    auto operator()(const T* v, size_t N) const
    {
        return iterable<const std::vector<T>, true, true>(v, N);
    }

    template <class T, size_t N>
    auto operator()(T (&v)[N]) const
    {
        return iterable<std::vector<T>, true, true>(v, N);
    }

    template <class T, size_t N>
    auto operator()(const T (&v)[N]) const
    {
        return iterable<const std::vector<T>, true, true>(v, N);
    }

    template <template <typename...> class C, class... Args>
    auto operator()(C<Args...>& v) const
    {
        static constexpr bool is_cont = is_contiguous<C<Args...>>::value;
        static constexpr bool is_ra = is_cont ? true : is_random_access<C<Args...>>::value;
        return iterable<C<Args...>, is_ra, is_cont>(v);
    }

    template <template <typename...> class C, class... Args>
    auto operator()(const C<Args...>& v) const
    {
        static constexpr bool is_cont = is_contiguous<C<Args...>>::value;
        static constexpr bool is_ra = is_cont ? true : is_random_access<C<Args...>>::value;
        return iterable<const C<Args...>, is_ra, is_cont>(v);
    }

    template <class C>
    auto operator()(C&, decltype(std::declval<C>().begin()) it0, decltype(std::declval<C>().begin()) it1) const
    {
        static constexpr bool is_cont = is_contiguous<C>::value;
        static constexpr bool is_ra = is_cont;
        return iterable<C, is_ra, is_cont>(it0, it1);
    }

    template <class C>
    auto operator()(const C&, decltype(std::declval<C>().begin()) it0, decltype(std::declval<C>().begin()) it1) const
    {
        static constexpr bool is_cont = is_contiguous<C>::value;
        static constexpr bool is_ra = is_cont;
        return iterable<const C, is_ra, is_cont>(it0, it1);
    }
};

static constexpr MakeIterable make_iterable;

struct ForEachIterable
{

    template <class ItSeq, class F>
    std::enable_if_t<!ItSeq::is_ra, void> operator()(ItSeq seq, F&& f) const
    {
        while (seq.has_next())
        {
            auto& e = seq.next();
            f(e);
        }
    }

    template <class ItSeq, class F>
    std::enable_if_t<ItSeq::is_ra, void> operator()(ItSeq seq, F&& f) const
    {
        for (size_t i = 0, N = seq.size(); i < N; ++i)
        {
            auto& e = seq.at(i);
            f(e);
        }
    }
};

static constexpr ForEachIterable for_each_iterable;

//========== functional ==================

template <class F, class A>

auto
bind_2nd(F&& f, A&& a)
{
    return [f = std::forward<F>(f), second = std::forward<A>(a)](auto&& first)
    {
        return f(std::forward<decltype(first)>(first), second);
    };
}

template <class F, class A>
auto
bind_1st(F&& f, A&& a)
{
    return [f = std::forward<F>(f), first = std::forward<A>(a)](auto&& second)
    {
        return f(first, std::forward<decltype(second)>(second));
    };
}

struct void_var_t
{
};

static constexpr void_var_t void_var;

template <class T, T v = T{}>
struct const_func {
    static constexpr T value = v;

    template <class... Args>
    const T& operator()(Args&&...) const
    {
        return value;
    }
};

template <class T>
auto make_const_func(const T& v)
{
    return [=](auto...) { return v; };
}

template <class T, class F>

std::enable_if_t<!std::is_same_v<T, void_var_t> && std::is_invocable_v<F, T>,
    std::conditional_t<std::is_void_v<std::invoke_result_t<F, T>>, void_var_t, std::invoke_result_t<F, T>>>
operator%(T&& arg, F&& f) // pipe operator
{
    if constexpr (std::is_void_v<std::invoke_result_t<F, T>>)
    {
        f(std::forward<T>(arg));
        return void_var;
    }
    else
        return f(std::forward<T>(arg));
}

template <class T, class F>
std::enable_if_t<std::is_same_v<T, void_var_t> && std::is_invocable_v<F>,
    std::conditional_t<std::is_void_v<std::invoke_result_t<F>>, void_var_t, std::invoke_result_t<F>>>
operator%(const T&, F&& f) // pipe operator

{
    if constexpr (std::is_void_v<std::invoke_result_t<F>>)
    {
        f();
        return void_var;
    }
    else
        return f();
}
} // namespace ftl
