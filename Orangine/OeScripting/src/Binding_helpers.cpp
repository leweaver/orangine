#include "pch.h"

#include <OeCore/Scene.h>
#include <OeScripting/Binding_helpers.h>

namespace py = pybind11;
using namespace oe;

Scene* Binding_helpers::get_scene() {
  // Get a pointer to the scene.
  const auto internalModule = py::module::import("engine_internal");
  Scene* scene = nullptr;
#ifdef _AMD64_
  uint64_t intPtr = internalModule.attr("scenePtr_uint64").cast<py::int_>();
  scene = reinterpret_cast<Scene*>(intPtr);
#elif _X86_
  uint32_t intPtr = internalModule.attr("scenePtr_uint32").cast<py::int_>();
  scene = reinterpret_cast<Scene*>(intPtr);
#else
#error Only AMD64 and x86 platforms are supported
#endif

  return scene;
}