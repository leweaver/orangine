#pragma once

#include "OeCore/Entity_filter.h"
#include "OeCore/IEntity_scripting_manager.h"

#include <Python.h>

namespace oe::internal {
class Entity_scripting_manager : public IEntity_scripting_manager {
 public:
  explicit Entity_scripting_manager(Scene& scene);

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override { return _name; }

  // Manager_tickable implementation
  void tick() override;

  // IEntity_scripting_manager implementation
  void renderImGui() override;
  void execute(const std::string& command) override;
  bool commandSuggestions(const std::string& command,
                          std::vector<std::string>& suggestions) override;

 private:
  const std::string _name = "Entity_scripting_manager";

  std::shared_ptr<Entity_filter> _scriptableEntityFilter;
  std::shared_ptr<Entity_filter> _testEntityFilter;
  std::shared_ptr<Entity_filter::Entity_filter_listener> _scriptableEntityFilterListener;
  std::vector<unsigned> _addedEntities;
  std::vector<unsigned> _removedEntities;

  std::shared_ptr<Entity_filter> _renderableEntityFilter;
  std::shared_ptr<Entity_filter> _lightEntityFilter;
  bool _showImGui = false;

  std::map<std::string, PyObject*> _loadedModules;
  std::wstring _pythonHome;
  std::wstring _pythonProgramName;

  // TODO: This would be part of a script context
  struct ScriptData {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = 10.0f;
  };
  ScriptData _scriptData;

  struct EngineInternalPythonModule {
    EngineInternalPythonModule(pybind11::module engineInternal);

    pybind11::module engine_internal;
    pybind11::detail::str_attr_accessor reset_output_streams;
  };
  struct PythonContext {
    pybind11::module sys;
    std::unique_ptr<EngineInternalPythonModule> engine_internal;
  };
  PythonContext _pythonContext;

  void initializePythonInterpreter();
  void finalizePythonInterpreter();
  void logPythonError(const pybind11::error_already_set& err, const std::string& whereStr) const;
  void flushStdIo();

  void renderDebugSpheres() const;

  bool _pythonInitialized = false;
};
} // namespace oe::internal
