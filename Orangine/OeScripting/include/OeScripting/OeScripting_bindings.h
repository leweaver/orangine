#pragma once

#include <pybind11/pybind11.h>

#include <OeCore/IInput_manager.h>
#include <OeCore/IScene_graph_manager.h>

namespace oe {
class OeScripting_bindings {
 public:
  OeScripting_bindings();
  void initializeSingletons(IInput_manager& inputManager, IScene_graph_manager& sceneGraphManager);

  static void create(pybind11::module& m);

  static void setModuleName(const char* string);
  static const std::string& getModuleName();

 private:
  pybind11::module _module;
  static std::string _moduleName;
};

class PyClass_input {
 public:
  explicit PyClass_input(IInput_manager& inputManager)
      : _inputManager(inputManager)
  {}

  IInput_manager::Mouse_state* getMouseState()
  {
    return _inputManager.getMouseState().lock().get();
  }

 private:
  IInput_manager& _inputManager;
};

class PyClass_sceneGraph {
 public:
  explicit PyClass_sceneGraph(IScene_graph_manager& sceneGraphManager)
      : _sceneGraphManager(sceneGraphManager)
  {}

  std::shared_ptr<Entity> instantiate(const std::string& name, Entity* parent = nullptr) {
    if (parent) {
      return _sceneGraphManager.instantiate(name, *parent);
    } else {
      return _sceneGraphManager.instantiate(name);
    }
  }

 private:
  IScene_graph_manager& _sceneGraphManager;
};
} // namespace oe
