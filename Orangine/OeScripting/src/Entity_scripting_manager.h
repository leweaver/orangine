#pragma once

#include <OeCore/Entity_filter.h>
#include <OeCore/IEntity_scripting_manager.h>
#include <OeCore/ITime_step_manager.h>

#include "Script_runtime_data.h"

namespace oe::internal {
class Entity_scripting_manager : public IEntity_scripting_manager {
 public:
  Entity_scripting_manager(Scene& scene, ITime_step_manager& timeStepManager);

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override { return _name; }

  // Manager_tickable implementation
  void tick() override;

  // IEntity_scripting_manager implementation
  void preInit_addAbsoluteScriptsPath(const std::wstring& path) override;
  void renderImGui() override;
  void execute(const std::string& command) override;
  bool commandSuggestions(const std::string& command,
                          std::vector<std::string>& suggestions) override;

  void loadSceneScript(const std::string& scriptClassString) override;

 private:
  const std::string _name = "Entity_scripting_manager";

  std::shared_ptr<Entity_filter> _testEntityFilter;

  std::shared_ptr<Entity_filter> _renderableEntityFilter;
  std::shared_ptr<Entity_filter> _lightEntityFilter;
  bool _showImGui = false;

  std::wstring _pythonHome;
  std::wstring _pythonProgramName;
  std::vector<std::wstring> _preInit_additionalPaths;

  struct EngineInternalPythonModule {
    EngineInternalPythonModule(pybind11::module engineInternal);

    pybind11::module instance;
    pybind11::detail::str_attr_accessor reset_output_streams;
    pybind11::detail::str_attr_accessor enable_remote_debugging;
  };

  struct PythonContext {
    pybind11::module _sysModule;
    std::unique_ptr<EngineInternalPythonModule> _engineInternalModule;
  };

  PythonContext _pythonContext;

  void initializePythonInterpreter();
  void finalizePythonInterpreter();
  static std::string logPythonError(const pybind11::error_already_set& err, const std::string& whereStr);
  void flushStdIo() const;
  void enableRemoteDebugging() const;

  void renderDebugSpheres() const;

  bool _pythonInitialized = false;
  std::vector < std::unique_ptr<Script_runtime_data>> _loadedScripts;

  ITime_step_manager& _timeStepManager;
};
} // namespace oe::internal
