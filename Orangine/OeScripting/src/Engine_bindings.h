#pragma once
#include <pybind11/pybind11.h>

namespace oe {
class Scene;

class Engine_bindings {
 public:
  static void create(pybind11::module& m);
};
} // namespace oe
