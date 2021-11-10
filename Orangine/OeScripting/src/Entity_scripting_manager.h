#pragma once

#include <OeCore/Entity_filter.h>
#include <OeCore/ITime_step_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/IAsset_manager.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IDev_tools_manager.h>

#include <OeScripting/IEntity_scripting_manager.h>

#include "Engine_bindings.h"
#include "Engine_internal_module.h"
#include "Script_runtime_data.h"

namespace oe::internal {
class Entity_scripting_manager : public IEntity_scripting_manager, public Manager_base, public Manager_tickable {
 public:
  Entity_scripting_manager(
          ITime_step_manager& timeStepManager, IScene_graph_manager& sceneGraphManager,
          IInput_manager& inputManager, IAsset_manager& assetManager, IEntity_render_manager& entityRenderManager);

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

  struct PythonContext {
    pybind11::module _sysModule;
    std::unique_ptr<Engine_internal_module> _engineInternalModule;
    std::unique_ptr<Engine_bindings> _oeModule;
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
  IScene_graph_manager& _sceneGraphManager;
  IInput_manager& _inputManager;
  IAsset_manager& _assetManager;
  IEntity_render_manager& _entityRenderManager;
};
} // namespace oe::internal
