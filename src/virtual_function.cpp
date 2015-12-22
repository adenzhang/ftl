#include <virtual_function.h>

namespace vf {

    void call(const VoidFunc& f) {
        f();
    }
} // namespace vf
