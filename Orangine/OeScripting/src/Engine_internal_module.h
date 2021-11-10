#pragma once

#include <pybind11/pybind11.h>

#include <OeCore/IInput_manager.h>

namespace oe {
/**
 * Helper accessors for the code defined in lib/engine_internal.py
 */
class Engine_internal_module {
 public:
  Engine_internal_module();

  pybind11::module instance;
  pybind11::detail::str_attr_accessor reset_output_streams;
  pybind11::detail::str_attr_accessor enable_remote_debugging;
};
}