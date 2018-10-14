#pragma once
#include <ftl/circular_queue.h>
#include <ftl/functional.h>
#include <setjmp.h>

#define TRY_                                             \
    do {                                                 \
        auto _try_jmp = !get_global_try_points().full(); \
        if (!_try_jmp || !sigsetjmp(get_global_try_points().emplace_back().jmpbuf, 1)) {

#define CATCH_ \
    }          \
    else       \
    {

#define TRY_END                             \
    }                                       \
    if (_try_jmp)                           \
        get_global_try_points().pop_back(); \
    }                                       \
    while (false)                           \
        ;

namespace ftl {

struct TryPoint {
    jmp_buf jmpbuf;
};

#define MAXTRY_DEPTH 64
using TryPointsQ = inline_circular_queue<TryPoint, MAXTRY_DEPTH>;

TryPointsQ& get_global_try_points();
void install_signal_catcher();
}
