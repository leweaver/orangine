#include "pch.h"

#include "Engine_internal_module.h"

namespace py = pybind11;

using namespace oe;

Engine_internal_module::Engine_internal_module()
    : instance(py::module::import("engine_internal"))
    , reset_output_streams(instance.attr("reset_output_streams"))
    , enable_remote_debugging(instance.attr("enable_remote_debugging"))
{
}
