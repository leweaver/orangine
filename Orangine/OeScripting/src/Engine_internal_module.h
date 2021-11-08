#pragma once

#include <pybind11/pybind11.h>

#include <OeCore/IInput_manager.h>

namespace oe {
class Engine_internal_module {
 public:
  Engine_internal_module();

  void initialize(IInput_manager& inputManager);
  IInput_manager& getInputManager();

  pybind11::module instance;
  pybind11::detail::str_attr_accessor reset_output_streams;
  pybind11::detail::str_attr_accessor enable_remote_debugging;
};
}