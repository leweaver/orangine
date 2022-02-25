// Must be included before any STL files
#include "OeScripting/OeScripting_bindings.h"

#include <OeCore/Entity.h>

#include <g3log/g3log.hpp>

namespace py = pybind11;
using oe::OeScripting_bindings;

namespace oe {

void engine_log_func(const std::string& message, const LEVELS& level)
{
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
  }
  else {
    LOG(level) << "[python] " << message;
  }
}

class PyClass_managers {
 public:
  explicit PyClass_managers(IInput_manager& inputManager, IScene_graph_manager& sceneGraphManager)
      : _input({inputManager}), _sceneGraph({sceneGraphManager}) {}

  PyClass_input& getInput()
  {
    return _input;
  }
  PyClass_sceneGraph& getSceneGraph() { return _sceneGraph; }

 private:
  PyClass_input _input;
  PyClass_sceneGraph _sceneGraph;
};
}// namespace oe


std::string OeScripting_bindings::_moduleName;

oe::OeScripting_bindings::OeScripting_bindings() : _module(py::module::import(_moduleName.c_str())) {}

void OeScripting_bindings::initializeSingletons(IInput_manager& inputManager, IScene_graph_manager& sceneGraphManager) {
   _module.attr("_runtime") = std::make_unique<PyClass_managers>(inputManager, sceneGraphManager);
}

void oe::OeScripting_bindings::setModuleName(const char* moduleName)
{
  _moduleName = moduleName;
}

const std::string& oe::OeScripting_bindings::getModuleName()
{
  return _moduleName;
}

void OeScripting_bindings::create(pybind11::module& m)
{
  m.doc() = "Orangine Core scripting API";

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
          .def(py::init<float, float, float>(), py::arg("x"), py::arg("y"), py::arg("z"))
          .def_property(
                  "x", [](const SSE::Vector3& v) { return static_cast<float>(v.getX()); },
                  [](SSE::Vector3& v, float val) { v.setX(val); })
          .def_property(
                  "y", [](const SSE::Vector3& v) { return static_cast<float>(v.getY()); },
                  [](SSE::Vector3& v, float val) { v.setY(val); })
          .def_property(
                  "z", [](const SSE::Vector3& v) { return static_cast<float>(v.getZ()); },
                  [](SSE::Vector3& v, float val) { v.setZ(val); });

  auto clsQuat = py::class_<SSE::Quat>(m, "Quat")
          // Default constructor; does no initialization
          .def(py::init<>())
          // Constructor with values
          .def(py::init<float, float, float, float>(), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"));
  clsQuat.def_property(
                 "x", [](const SSE::Quat& v) { return static_cast<float>(v.getX()); },
                 [](SSE::Quat& v, float val) { v.setX(val); })
          .def_property(
                  "y", [](const SSE::Quat& v) { return static_cast<float>(v.getY()); },
                  [](SSE::Quat& v, float val) { v.setY(val); })
          .def_property(
                  "z", [](const SSE::Quat& v) { return static_cast<float>(v.getZ()); },
                  [](SSE::Quat& v, float val) { v.setZ(val); })
          .def_property(
                  "w", [](const SSE::Quat& v) { return static_cast<float>(v.getW()); },
                  [](SSE::Quat& v, float val) { v.setW(val); })          //.def_static("rotation_x", static_cast<const SSE::Quat (*)(float)>(&SSE::Quat::rotationX), py::arg("radians"))
          .def("from_rotation_x", [](SSE::Quat& self, float radians) { return Quat::rotationX(radians); }, py::arg("radians"))
          .def("from_rotation_y", [](SSE::Quat& self, float radians) { return Quat::rotationY(radians); }, py::arg("radians"))
          .def("from_rotation_z", [](SSE::Quat& self, float radians) { return Quat::rotationZ(radians); }, py::arg("radians"))
          .def("__mul__", [](const SSE::Quat& lhs, const SSE::Quat& rhs) { return lhs * rhs; });
  // Entity
  py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity")
          .def("get_name", &Entity::getName)
          .def_property("position", &Entity::position, &Entity::setPosition)
          .def_property("rotation", &Entity::rotation, &Entity::setRotation)
          .def_property("scale", &Entity::scale, static_cast<void (Entity::*)(const SSE::Vector3&)>(&Entity::setScale))
          .def_property("scale", &Entity::scale, static_cast<void (Entity::*)(float)>(&Entity::setScale));

  // Input Manager
  auto clsInput = py::class_<PyClass_input>(m, "Input");
  py::enum_<IInput_manager::Mouse_state::Button_state>(clsInput, "MouseButtonState")
          .value("Up", IInput_manager::Mouse_state::Button_state::Up)
          .value("Held", IInput_manager::Mouse_state::Button_state::Held)
          .value("Released", IInput_manager::Mouse_state::Button_state::Released)
          .value("Pressed", IInput_manager::Mouse_state::Button_state::Pressed)
          .export_values();

  py::class_<IInput_manager::Mouse_state>(clsInput, "MouseState")
          .def(py::init<>())
          .def_readwrite("left", &IInput_manager::Mouse_state::left)
          .def_readwrite("middle", &IInput_manager::Mouse_state::middle)
          .def_readwrite("right", &IInput_manager::Mouse_state::right)
          .def_readwrite("delta_position", &IInput_manager::Mouse_state::deltaPosition)
          .def_readwrite("scroll_wheel_delta", &IInput_manager::Mouse_state::scrollWheelDelta);

  //.def(py::init<>()) Don't allow construction from Python
  clsInput.def("get_mouse_state", &PyClass_input::getMouseState, py::return_value_policy::copy);

  // SceneGraph Manager
  const auto clsSceneGraph = py::class_<PyClass_sceneGraph>(m, "SceneGraph")
          .def("instantiate", &PyClass_sceneGraph::instantiate, py::arg("name") = "", py::arg("parent") = py::none());

  py::class_<PyClass_managers>(m, "NativeContext")
          .def_property_readonly("input", &PyClass_managers::getInput)
          .def_property_readonly("scene_graph", &PyClass_managers::getSceneGraph);

  // This contains a PyClass_managers instance
  m.def("runtime", [m] { return m.attr("_runtime").cast<PyClass_managers>(); });
}
