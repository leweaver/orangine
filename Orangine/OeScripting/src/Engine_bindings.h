#pragma once
#include <pybind11/pybind11.h>

namespace oe {
class Scene;

class Engine_bindings {
 public:
  static void create(pybind11::module& m);
  static void setScene(Scene* scene) { _scene = scene; }

 private:
  static Scene* _scene;
};
} // namespace oe
