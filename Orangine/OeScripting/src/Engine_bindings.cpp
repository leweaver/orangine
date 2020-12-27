#include "pch.h"

#include "Engine_bindings.h"

#include "OeCore/Scene.h"
#include "OeScripting/Binding_helpers.h"

namespace py = pybind11;
using oe::Engine_bindings;

namespace oe {

void engine_log_func(const std::string& message, const LEVELS& level) {
  if (!g3::logLevel(level)) {
    return;
  }

  const auto inspectModule = py::module::import("inspect");
  const auto stackArray = inspectModule.attr("stack")();
  if (py::isinstance<py::list>(stackArray)) {
    const auto stackList = py::list(stackArray);
    if (!stackList.empty()) {
      const auto frame = stackList[0];

      auto filename = std::string(py::str(frame.attr("filename")));
      const auto lastSlash = std::max(filename.find_last_of('/'), filename.find_last_of('\\'));
      if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash);
      }
      const auto lineNo = frame.attr("lineno").cast<int>();
      const auto function = std::string(py::str(frame.attr("function")));

      LogCapture(filename.c_str(), lineNo, function.c_str(), level).stream() << message;
    }
  } else {
    LOG(level) << "[python] " << message;
  }
}

class PyClass_mouse_state {};

class PyClass_input {
 public:
  PyClass_input() { _scene = Binding_helpers::get_scene(); }

  IInput_manager::Mouse_state* getMouseState() {
    return _scene->manager<IInput_manager>().getMouseState().lock().get();
  }

 private:
  Scene* _scene;
};

class PyClass_managers {
 public:
  PyClass_managers() = default;

  PyClass_input& getInput() { return _input; }

 private:
  PyClass_input _input;
};
} // namespace oe

void Engine_bindings::create(pybind11::module& m) {
  // Logging
  m.def("log_debug_enabled", []() { return g3::logLevel(G3LOG_DEBUG); });
  m.def("log_debug", [](const std::string& message) { engine_log_func(message, G3LOG_DEBUG); });

  m.def("log_info_enabled", []() { return g3::logLevel(INFO); });
  m.def("log_info", [](const std::string& message) { engine_log_func(message, INFO); });

  m.def("log_warning_enabled", []() { return g3::logLevel(WARNING); });
  m.def("log_warning", [](const std::string& message) { engine_log_func(message, WARNING); });

  // Renderer types
  py::class_<Vector2u>(m, "Vector2u")
      .def(py::init<>())
      .def_readwrite("x", &Vector2u::x)
      .def_readwrite("y", &Vector2u::y);
  py::class_<Vector2i>(m, "Vector2i")
      .def(py::init<>())
      .def_readwrite("x", &Vector2i::x)
      .def_readwrite("y", &Vector2i::y);

  // Input Manager
  const auto clsInput =
      pybind11::class_<PyClass_input>(m, "Input")
          .def(py::init<>())
          .def("getMouseState", &PyClass_input::getMouseState, py::return_value_policy::copy);

  py::enum_<IInput_manager::Mouse_state::Button_state>(clsInput, "MouseButtonState")
      .value("Up", IInput_manager::Mouse_state::Button_state::Up)
      .value("Held", IInput_manager::Mouse_state::Button_state::Held)
      .value("Released", IInput_manager::Mouse_state::Button_state::Released)
      .value("Pressed", IInput_manager::Mouse_state::Button_state::Pressed)
      .export_values();

  pybind11::class_<IInput_manager::Mouse_state>(clsInput, "MouseState")
      .def(py::init<>())
      .def_readwrite("left", &IInput_manager::Mouse_state::left)
      .def_readwrite("middle", &IInput_manager::Mouse_state::middle)
      .def_readwrite("right", &IInput_manager::Mouse_state::right)
      .def_readwrite("delta_position", &IInput_manager::Mouse_state::deltaPosition)
      .def_readwrite("scroll_wheel_delta", &IInput_manager::Mouse_state::scrollWheelDelta);

  // Managers
  py::class_<PyClass_managers>(m, "Managers")
                         .def_property_readonly("input", &PyClass_managers::getInput);

  m.def("init_statics", [m]() { m.attr("managers") = new PyClass_managers(); });
}