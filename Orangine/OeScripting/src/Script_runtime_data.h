#pragma once

#include <pybind11/pytypes.h>

namespace oe {
    namespace internal {
        struct Script_runtime_data {
            pybind11::object instance;
            bool hasTick = false;
        };
    }
}