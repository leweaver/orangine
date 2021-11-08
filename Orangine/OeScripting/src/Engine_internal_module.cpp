#include "pch.h"

#include "Engine_internal_module.h"

namespace py = pybind11;

using namespace oe;

namespace oe {
class PyClass_native_context {
 public:
  PyClass_native_context(IInput_manager& inputManager)
      : _inputManager(inputManager) {}

  IInput_manager& getInputManager()
  {
    return _inputManager;
  }

 private:
  IInput_manager& _inputManager;
};
}// namespace oe

Engine_internal_module::Engine_internal_module()
    : instance(py::module::import("engine_internal"))
    , reset_output_streams(instance.attr("reset_output_streams"))
    , enable_remote_debugging(instance.attr("enable_remote_debugging"))
{
  py::class_<PyClass_native_context>(instance, "NativeContext");
}

void Engine_internal_module::initialize(IInput_manager& inputManager)
{
  instance.attr("native_context") = std::make_unique<PyClass_native_context>(inputManager);
}

IInput_manager& Engine_internal_module::getInputManager()
{
  return instance.attr("native_context").cast<PyClass_native_context>().getInputManager();
}
