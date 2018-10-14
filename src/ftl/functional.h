#pragma once
#include <utility>

namespace ftl {

template <typename F>
struct scope_exit {
    scope_exit(F&& f) : f(std::forward<F>(f)) {}
    ~scope_exit() { if( !mReleased) f(); }

    void release() {
        mReleased = true;
    }
protected:
    F f;
    bool mReleased = false;
};

}
