#include "pch.h"

#include "Engine_bindings.h"

#include "OeCore/Scene.h"
#include "OeScripting/Binding_helpers.h"

namespace py = pybind11;
using oe::Engine_bindings;

namespace oe {

Scene* Engine_bindings::_scene = nullptr;

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

  // Math types
  py::class_<SSE::Vector3>(m, "Vector3")
      // Default constructor; does no initialization
      .def(py::init<>())
      // Constructor with values
      .def(py::init<float, float, float>())
      .def_property(
          "x",
          [](const SSE::Vector3& v) { return static_cast<float>(v.getX()); },
          [](SSE::Vector3& v, float val) { v.setX(val); })
      .def_property(
          "y",
          [](const SSE::Vector3& v) { return static_cast<float>(v.getY()); },
          [](SSE::Vector3& v, float val) { v.setY(val); })
      .def_property(
          "z",
          [](const SSE::Vector3& v) { return static_cast<float>(v.getZ()); },
          [](SSE::Vector3& v, float val) { v.setZ(val); });

  py::class_<SSE::Quat>(m, "Quat")
      // Default constructor; does no initialization
      .def(py::init<>())
      // Constructor with values
      .def(py::init<float, float, float, float>())
      .def_property(
          "x",
          [](const SSE::Quat& v) { return static_cast<float>(v.getX()); },
          [](SSE::Quat& v, float val) { v.setX(val); })
      .def_property(
          "y",
          [](const SSE::Quat& v) { return static_cast<float>(v.getY()); },
          [](SSE::Quat& v, float val) { v.setY(val); })
      .def_property(
          "z",
          [](const SSE::Quat& v) { return static_cast<float>(v.getZ()); },
          [](SSE::Quat& v, float val) { v.setZ(val); })
      .def_property(
          "w",
          [](const SSE::Quat& v) { return static_cast<float>(v.getW()); },
          [](SSE::Quat& v, float val) { v.setW(val); })
      .def_static("rotation_x", static_cast<const Quat (*)(float)>(&Quat::rotationX))
      .def_static("rotation_y", static_cast<const Quat (*)(float)>(&Quat::rotationY))
      .def_static("rotation_z", static_cast<const Quat (*)(float)>(&Quat::rotationZ))
      .def("__mul__", [](const Quat& lhs, const Quat& rhs)
      {
        return lhs * rhs;
      });

  // Entity
  py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity")
      .def("get_name", &Entity::getName)
      .def_property("position", &Entity::position, &Entity::setPosition)
      .def_property("rotation", &Entity::rotation, &Entity::setRotation)
      .def_property(
          "scale",
          &Entity::scale,
          static_cast<void (Entity::*)(const SSE::Vector3&)>(&Entity::setScale))
      .def_property(
          "scale", &Entity::scale, static_cast<void (Entity::*)(float)>(&Entity::setScale));

  // Scene
  py::class_<Scene>(m, "Scene")
      .def("get_main_camera", &Scene::getMainCamera)
      .def("set_main_camera", &Scene::setMainCamera);

  // Input Manager
  const auto clsInput =
      pybind11::class_<PyClass_input>(m, "Input")
          .def(py::init<>())
          .def("get_mouse_state", &PyClass_input::getMouseState, py::return_value_policy::copy);

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

  m.def("init_statics", [m]() { m.attr("managers") = std::make_unique<PyClass_managers>(); });
  m.def(
      "get_scene", []() { return _scene; }, py::return_value_policy::reference);

  LOG(INFO) << "Created engine bindings";
}