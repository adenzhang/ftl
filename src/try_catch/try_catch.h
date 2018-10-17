#ifndef FTL_TRY_CATCH_H
#define FTL_TRY_CATCH_H

#include <array>
#include <cstdio>
#include <setjmp.h>
#include <stdarg.h>

/// comment/uncommnet FTL_TRY_ENABLE to disable/enable FTL_TRY system.
#define FTL_TRY_ENABLE

/////////////////////////////////////////////////////////////////////////
/// users are supposed to use only the below macro
///

#ifdef FTL_TRY_ENABLE

#define FTL_TRY_INSTALL() _install_try_catch()
#define FTL_TRY_UNINSTALL() _uninstall_try_catch()

#define FTL_TRY_(pPrintf)                \
    do {                                 \
        const _AutoTryPointPop _autopop; \
        if (!_autopop.bPop || !sigsetjmp(_get_thread_try_points().emplace(pPrintf)->jmpbuf, 1)) {

#define FTL_CATCH \
    }             \
    else          \
    {

#define FTL_TRY_END \
    }               \
    }               \
    while (false)   \
        ;

#else
///================== FTL_TRY is disabled

#define FTL_TRY_INSTALL()
#define FTL_TRY_UNINSTALL()
#define FTL_TRY_(pPrintf) \
    if (true) {
#define FTL_CATCH \
    }             \
    else          \
    {
#define FTL_TRY_END }

#endif

#define FTL_TRY FTL_TRY_(nullptr)

#define FTL_TRY_LOG(printf) FTL_TRY_(&printf)

#define MAX_TRY_DEPTH 64

///
/// users are supposed to use only the above macro.
/// below are util class/functions
/////////////////////////////////////////////////////////////////////////

struct IPrintf {
    void operator()(const char* format, ...)
    {
        va_list arglist;
        va_start(arglist, format);
        vprint(format, arglist);
        va_end(arglist);
    }
    void printf(const char* format, ...)
    {
        va_list arglist;
        va_start(arglist, format);
        vprint(format, arglist);
        va_end(arglist);
    }
    // todo: void print(... anytype...)
    // todo: void join(",", ...anytype...)
    virtual ~IPrintf() = default;

protected:
    virtual void vprint(const char* format, va_list args) = 0;
};

struct NullPrintf : public IPrintf {
protected:
    void vprint(const char*, va_list) override
    {
    }
};

static NullPrintf nullPrintf;

struct FilePrintf : public IPrintf {
    FilePrintf(FILE* f = stdout)
        : mFile(f)
    {
    }

    void close()
    {
        if (mFile) {
            fclose(mFile);
            mFile = nullptr;
        }
    }

protected:
    FILE* mFile = nullptr;
    void vprint(const char* format, va_list args) override
    {
        vfprintf(mFile, format, args);
        fflush(mFile);
    }
};

template <size_t N>
struct BufferPrintf : public IPrintf {
    void clear()
    {
        len = 0;
        buf[0] = 0;
    }
    const char* c_str() const
    {
        return buf;
    }

protected:
    char buf[N];
    size_t len = 0;

    void vprint(const char* format, va_list args) override
    {
        if (N > len)
            len += vsnprintf(buf + len, N - len, format, args);
    }
};

static FilePrintf stdoutPrintf;

template <class DataT = IPrintf*>
struct thread_try_points_base {
    struct try_point_data {
        DataT data;
        jmp_buf jmpbuf;
        void clear()
        {
            memset(this, 0, sizeof(try_point_data));
        }
    };

    try_point_data* mTryPoints = nullptr;
    size_t mCap = 0;
    size_t mSize = 0;

    thread_try_points_base(try_point_data* p = nullptr, size_t cap = 0)
        : mTryPoints(p)
        , mCap(cap)
    {
    }
    ~thread_try_points_base()
    {
    }
    inline size_t size() const
    {
        return mSize;
    }
    inline try_point_data* emplace(const DataT& data = {})
    {
        if (size() == mCap)
            return nullptr;
        auto& d = mTryPoints[mSize++];
        d.data = data;
        return &d;
    }

    inline void pop()
    {
        if (mSize > 0)
            --mSize;
    }
    inline bool empty() const
    {
        return !mSize;
    }
    inline bool full() const
    {
        return mSize == mCap;
    }

    inline try_point_data* top()
    {
        return mSize ? &mTryPoints[mSize - 1] : nullptr;
    }
    inline void jump2top()
    {
        siglongjmp(mTryPoints[mSize - 1].jmpbuf, 1);
    }
};
using thread_try_points_base_t = thread_try_points_base<>;

template <size_t N>
struct thread_try_points : public thread_try_points_base_t {
    using Data = std::array<try_point_data, N>;
    Data mData;

    thread_try_points()
        : thread_try_points_base()
    {
        mTryPoints = &mData[0];
        mCap = N;
        mSize = 0;
    }
};

#define EXPORT_API __attribute__((visibility("default")))

#define DECL_get_global_try_points() EXPORT_API thread_try_points_base_t& _get_thread_try_points()

// strictly use this macro no more than once in a program
#define IMPL_get_global_try_points(TRY_POINT_DEPTH)                          \
    thread_try_points_base_t& _get_thread_try_points()                       \
    {                                                                        \
        static thread_local thread_try_points<TRY_POINT_DEPTH> s_try_points; \
        return s_try_points;                                                 \
    }

DECL_get_global_try_points();

EXPORT_API bool _is_try_catch_installed();
EXPORT_API void _install_try_catch(int array_or_stack_jmp = 0, bool bForce = false); // array_or_stack_jmp: 0, 1; force reinstall only if bForce
EXPORT_API void _uninstall_try_catch();
EXPORT_API void _print_stack_trace(IPrintf* fmt);
EXPORT_API void _set_thread_stack_printer(IPrintf* fmt);

struct _AutoTryPointPop {
    const bool bPop;
    _AutoTryPointPop()
        : bPop(_is_try_catch_installed() && !_get_thread_try_points().full())
    {
    }
    inline ~_AutoTryPointPop()
    {
        if (bPop) {
            _get_thread_try_points().pop();
        }
    }
};

/////////////////////////   stack jump /////////////////////////

struct try_point_data {
    IPrintf* data = nullptr;
    jmp_buf jmpbuf;
};

EXPORT_API try_point_data& _get_thread_try_point();

struct TryPointRestore {
    try_point_data d;
    TryPointRestore(const try_point_data& a)
        : d(a)
    {
    }
    ~TryPointRestore()
    {
        _get_thread_try_point() = d;
    }
};

#define FTL_TRY1                                            \
    do {                                                    \
        try_point_data& _currtry = _get_thread_try_point(); \
        TryPointRestore _autorestore(_currtry);             \
        if (!sigsetjmp(_currtry.jmpbuf, 1)) {

#define FTL_CATCH1 \
    }              \
    else           \
    {

#define FTL_TRY_END1 \
    }                \
    }                \
    while (false)    \
        ;

#define FTL_TRY_INSTALL1() _install_try_catch(1)
#endif
