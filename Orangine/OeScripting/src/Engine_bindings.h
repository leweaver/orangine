#pragma once
#include <OeCore/IInput_manager.h>
#include <pybind11/pybind11.h>

namespace oe {
class Scene;

class Engine_bindings {
 public:
  explicit Engine_bindings(pybind11::module module) : _module(module) {}
  void initializeSingletons(IInput_manager& inputManager);

  static void create(pybind11::module& m);

 private:
  pybind11::module _module;
};
} // namespace oe
