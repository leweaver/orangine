#pragma once

#include <OeCore/Entity_filter.h>
#include <OeCore/ITime_step_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/IAsset_manager.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IDev_tools_manager.h>

#include <OeScripting/IEntity_scripting_manager.h>
#include <OeScripting/OeScripting_bindings.h>

#include "Script_runtime_data.h"

namespace oe::internal {
class Entity_scripting_manager : public IEntity_scripting_manager, public Manager_base, public Manager_tickable {
 public:
  Entity_scripting_manager(
          ITime_step_manager& timeStepManager, IScene_graph_manager& sceneGraphManager,
          IInput_manager& inputManager, IAsset_manager& assetManager, IEntity_render_manager& entityRenderManager);

  // Manager_base implementation
  void loadConfig(const IConfigReader& configReader) override;
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

  void loadSceneScript(const std::string& scriptClassString) override;

 private:
  const std::string _name = "Entity_scripting_manager";

  std::shared_ptr<Entity_filter> _testEntityFilter;

  std::shared_ptr<Entity_filter> _renderableEntityFilter;
  std::shared_ptr<Entity_filter> _lightEntityFilter;
  bool _showImGui = false;

  std::string _pyenvPath;
  std::vector<std::string> _scriptRoots;

  /**
   * Helper accessors for the code defined in lib/engine_internal.py
   */
  class Engine_internal_module {
   public:
    Engine_internal_module();

    pybind11::module instance;
    pybind11::detail::str_attr_accessor reset_output_streams;
    pybind11::detail::str_attr_accessor enable_remote_debugging;
  };

  struct PythonContext {
    pybind11::module _sysModule;
    std::unique_ptr<Engine_internal_module> _engineInternalModule;
    std::unique_ptr<OeScripting_bindings> _oeModule;
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
