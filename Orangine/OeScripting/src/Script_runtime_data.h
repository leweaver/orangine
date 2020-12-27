#pragma once

#include <pybind11/pytypes.h>

namespace oe {
namespace internal {
struct Script_runtime_data {
  std::string scriptName;
  pybind11::object instance;
  bool hasTick = false;
  bool hasInit = false;
};
} // namespace internal
} // namespace oe