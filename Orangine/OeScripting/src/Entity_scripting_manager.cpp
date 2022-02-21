#include "pch.h"

#include "Entity_scripting_manager.h"

#include <OeCore/Light_component.h>
#include <OeCore/Test_component.h>

#include <imgui.h>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace py = pybind11;

using namespace DirectX;
using namespace oe;
using namespace oe::internal;

#ifdef OE_PYTHON_DEBUG
#ifndef Py_DEBUG
// Well this was an evening wasted! When linking against a debug build of python, you must also set
// this flag in the consuming application.
#error Py_DEBUG must be defined if linking with debug python library.
#endif
#endif

template<>
void oe::create_manager(Manager_instance<IEntity_scripting_manager>& out,
        ITime_step_manager& timeStepManager, IScene_graph_manager& sceneGraphManager,
        IInput_manager& inputManager, IAsset_manager& assetManager, IEntity_render_manager& entityRenderManager)
{
  out = Manager_instance<IEntity_scripting_manager>(std::make_unique<Entity_scripting_manager>(
          timeStepManager, sceneGraphManager, inputManager, assetManager, entityRenderManager));
}

Entity_scripting_manager::Engine_internal_module::Engine_internal_module()
    : instance(py::module::import("engine_internal"))
    , reset_output_streams(instance.attr("reset_output_streams"))
    , enable_remote_debugging(instance.attr("enable_remote_debugging"))
{
}

Entity_scripting_manager::Entity_scripting_manager(
        ITime_step_manager& timeStepManager, IScene_graph_manager& sceneGraphManager, IInput_manager& inputManager,
        IAsset_manager& assetManager, IEntity_render_manager& entityRenderManager)
    : Manager_base()
    , Manager_tickable()
    , _timeStepManager(timeStepManager)
    , _sceneGraphManager(sceneGraphManager)
    , _inputManager(inputManager)
    , _assetManager(assetManager)
    , _entityRenderManager(entityRenderManager)
{}

void Entity_scripting_manager::preInit_addAbsoluteScriptsPath(const std::string& path)
{
  if (_pythonInitialized) {
    OE_THROW(std::logic_error("preInit_addAbsoluteScriptsPath can only be called prior to manager initialization."));
  }
  if (!std::filesystem::exists(path)) {
    OE_THROW(std::runtime_error(
            "Attempting to add python script path that does not exist, or is inaccessible: " + path));
  }
  _preInit_additionalPaths.push_back(path);
}

void Entity_scripting_manager::initialize()
{
  _testEntityFilter = _sceneGraphManager.getEntityFilter({Test_component::type()});

  // TODO: can't inlclude renderable_component right now
  /*
  _renderableEntityFilter =
      _sceneGraphManager.getEntityFilter({Renderable_component::type()});
      */
  _lightEntityFilter = _sceneGraphManager.getEntityFilter(
          {Directional_light_component::type(), Point_light_component::type(), Ambient_light_component::type()},
          Entity_filter_mode::Any);

  std::string errorStr;
  try {
    initializePythonInterpreter();
    return;
  }
  catch (const py::error_already_set& err) {
    errorStr = logPythonError(err, "service initialization");
  }

  // This must happen outside of a catch, so that the error_already_set object is destructed prior
  // to throwing.
  finalizePythonInterpreter();
  throw ::std::runtime_error(errorStr);
}

std::string Entity_scripting_manager::logPythonError(const py::error_already_set& err, const std::string& whereStr)
{
  std::stringstream ss;
  ss << "Python error in " << whereStr << ": " << std::string(py::str(err.value()));

  const auto errorString = ss.str();

  auto traceback = err.trace();
  while (!traceback.is_none() && traceback.ptr() != nullptr) {
    std::string lineno = std::string(py::str(traceback.attr("tb_lineno")));
    std::string filename = std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_filename")));
    std::string funcname = std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_name")));

    ss << std::endl;
    ss << " at " << filename << ":" << lineno << " -> " << funcname;

    traceback = traceback.attr("tb_next");
  }

  LOG(WARNING) << ss.str();
  return errorString;
}

void Entity_scripting_manager::tick()
{
  for (auto& runtimeDataPtr : _loadedScripts) {
    assert(runtimeDataPtr.get());
    auto& runtimeData = *runtimeDataPtr;

    if (!runtimeData.hasTick) continue;

    try {
      auto _ = runtimeData.instance.attr("tick")();
    }
    catch (const py::error_already_set& err) {
      logPythonError(err, "Native::tick() " + runtimeData.scriptName);
    }
  }

  try {
    flushStdIo();
  }
  catch (const py::error_already_set& err) {
    logPythonError(err, "Native::tick() - Flushing stdout and stderr");
  }

  const auto elapsedTime = _timeStepManager.getElapsedTime();
  for (auto iter = _testEntityFilter->begin(); iter != _testEntityFilter->end(); ++iter) {
    auto& entity = **iter;
    auto* component = entity.getFirstComponentOfType<Test_component>();
    if (component == nullptr) continue;

    const auto& speed = component->getSpeed();

    const auto animTimePitch =
            SSE::Quat::rotationX(static_cast<float>(fmod(elapsedTime * speed.getX() * XM_2PI, XM_2PI)));
    const auto animTimeYaw =
            SSE::Quat::rotationY(static_cast<float>(fmod(elapsedTime * speed.getY() * XM_2PI, XM_2PI)));
    const auto animTimeRoll =
            SSE::Quat::rotationZ(static_cast<float>(fmod(elapsedTime * speed.getZ() * XM_2PI, XM_2PI)));
    entity.setRotation(animTimeYaw * animTimePitch * animTimeRoll);
  }

  /*
  def init(self):
    _scriptData.yaw = XM_PI;

  def tick(self):
    const auto mouseSpeed = 1.0f / 600.0f;
    const auto mouseState = _scene.manager<IInput_manager>().mouseState().lock();
    if (mouseState) {
      if (mouseState->left == IInput_manager::Mouse_state::Button_state::HELD) {
        _scriptData.yaw += -mouseState->deltaPosition.x * XM_2PI * mouseSpeed;
        _scriptData.pitch += mouseState->deltaPosition.y * XM_2PI * mouseSpeed;

        _scriptData.pitch = std::max(XM_PI * -0.45f, std::min(XM_PI * 0.45f, _scriptData.pitch));
      }
      // if (mouseState->middle == Input_manager::Mouse_state::Button_state::HELD) {
      _scriptData.distance = std::max(
          1.0f,
          std::min(
              40.0f,
              _scriptData.distance + static_cast<float>(mouseState->scrollWheelDelta) *
  -mouseSpeed));
      //}

      // if (mouseState->right == IInput_manager::Mouse_state::Button_state::HELD) {
      //  renderDebugSpheres();
      //}

      const auto cameraRot = SSE::Matrix4::rotationY(_scriptData.yaw + math::pi) *
                             SSE::Matrix4::rotationX(-_scriptData.pitch);
      auto cameraPosition = cameraRot * math::backward;
      cameraPosition *= _scriptData.distance;
  }
  */
}

void Entity_scripting_manager::loadSceneScript(const std::string& scriptClassString)
{

  if (scriptClassString.empty()) {
    LOG(WARNING) << "Invalid script name, cannot be empty";
    return;
  }
  if (scriptClassString.at(scriptClassString.size() - 1) == '.') {
    LOG(WARNING) << "Invalid script name, cannot end with .";
    return;
  }

  try {
    const auto lastDot = scriptClassString.find_last_of('.');
    auto runtimeDataPtr = std::make_unique<Script_runtime_data>();
    runtimeDataPtr->scriptName = scriptClassString;

    // Load the python class/module and create an instance.
    if (lastDot != std::string::npos) {
      const auto scriptModuleName = scriptClassString.substr(0, lastDot);
      const auto scriptClassName = scriptClassString.substr(lastDot + 1);

      const auto scriptModule = py::module::import(scriptModuleName.c_str());
      const auto scriptClassDefn = scriptModule.attr(scriptClassName.c_str());

      runtimeDataPtr->instance = scriptClassDefn();
    }
    else {
      const auto scriptClassDefn = py::globals().attr(scriptClassString.c_str());

      runtimeDataPtr->instance = scriptClassDefn();
    }

    runtimeDataPtr->hasInit = py::hasattr(runtimeDataPtr->instance, "init");
    runtimeDataPtr->hasTick = py::hasattr(runtimeDataPtr->instance, "tick");

    _loadedScripts.emplace_back(std::move(runtimeDataPtr));
  }
  catch (const py::error_already_set& err) {
    logPythonError(err, "Native::loadSceneScript create - " + scriptClassString);
    return;
  }

  try {
    // If required, initialize the via the init method instance
    auto* const runtimeData = _loadedScripts.back().get();
    assert(runtimeData->scriptName == scriptClassString);

    if (runtimeData->hasInit) {
      auto _ = runtimeData->instance.attr("init")();
    }
  }
  catch (const py::error_already_set& err) {
    logPythonError(err, "Native::loadSceneScript init - " + scriptClassString);
  }
}

void Entity_scripting_manager::flushStdIo() const
{
  const auto _ = _pythonContext._engineInternalModule->reset_output_streams();
}

void Entity_scripting_manager::enableRemoteDebugging() const
{
  const auto _ = _pythonContext._engineInternalModule->enable_remote_debugging();
}

void Entity_scripting_manager::shutdown()
{
  finalizePythonInterpreter();
}

void Entity_scripting_manager::initializePythonInterpreter()
{
  py::initialize_interpreter();

  _pythonInitialized = true;

  std::vector<std::string> sysPathVec;

  const auto& dataLibPath = _assetManager.makeAbsoluteAssetPath("OeScripting/lib");
  sysPathVec.push_back(dataLibPath);

  const auto& pyEnvPath = _assetManager.makeAbsoluteAssetPath("pyenv") + "/..";

  // Add pyenv scripts
  sysPathVec.push_back(pyEnvPath + "/lib");
  sysPathVec.push_back(pyEnvPath + "/lib/site-packages");

  // Add system installed python (for non-installed, dev machine builds only). Must come after pyenv
  auto pyenvCfgPath = pyEnvPath + "/pyvenv.cfg";
  if (std::filesystem::exists(pyenvCfgPath)) {
    std::string line;
    std::ifstream infile(pyenvCfgPath, std::ios::in);
    while (std::getline(infile, line)) {
      auto equalPos = line.find_first_of('=');
      if (std::string::npos == equalPos) {
        LOG(DEBUG) << "Failed to parse line in " << pyenvCfgPath << ": " << line;
        continue;
      }

      std::string test1 = str_trim("   hello");
      std::string test2 = str_trim("   hello   ");
      std::string test3 = str_trim("hello   ");
      std::string test4 = str_trim("");

      std::string propName = str_trim(line.substr(0, equalPos - 1));
      std::string value = str_trim(line.substr(equalPos + 1));

      if (propName == "home") {
        LOG(DEBUG) << "Detected python home from " << pyenvCfgPath << ": " << value;
        sysPathVec.push_back(value + "/Lib");
        sysPathVec.push_back(value + "/DLLs");
      }
      if (propName == "include-system-site-packages" && value == "true") {
        LOG(DEBUG) << "Detected include-system-site-packages from " << pyenvCfgPath;
        sysPathVec.push_back(value + "/Lib/site-packages");
      }
    }
  }

  // Add additional user script paths
  std::copy(_preInit_additionalPaths.begin(), _preInit_additionalPaths.end(), std::back_inserter(sysPathVec));

  // Convert to a python list, and overwrite the existing value of sys.path
  auto sysPathList = py::list();
  std::stringstream sysPathListSs;
  for (int i = 0, e = static_cast<int>(sysPathVec.size()); i < e; ++i) {
    const auto path = std::filesystem::absolute(sysPathVec[i]).u8string();
    sysPathList.append(path);
    if (i) {
      sysPathListSs << ";";
    }
    sysPathListSs << path;
  }

  LOG(INFO) << "Python path set to: " << sysPathListSs.str();

  _pythonContext._sysModule = py::module::import("sys");
  _pythonContext._sysModule.attr("path") = sysPathList;

  _pythonContext._oeModule = std::make_unique<OeScripting_bindings>(py::module::import("oe"));
  _pythonContext._oeModule->initializeSingletons(_inputManager);

  _pythonContext._engineInternalModule = std::make_unique<Engine_internal_module>();

  // This must be called at least once before python attempts to write to stdout or stderr.
  // Now that we have our internal library, set up stdout and stderr properly.
  flushStdIo();

  enableRemoteDebugging();

  auto initFn = _pythonContext._engineInternalModule->instance.attr("init");
  auto _ = initFn();
}

void Entity_scripting_manager::finalizePythonInterpreter()
{
  _loadedScripts.clear();
  _pythonContext = {};

  if (_pythonInitialized) {
    // This must happen last, after all our handles to python objects are cleared.
    py::finalize_interpreter();
    _pythonInitialized = false;
  }
}

void Entity_scripting_manager::renderImGui()
{
  if (_showImGui) {
    ImGui::ShowDemoWindow();
  }
}

void Entity_scripting_manager::execute(const std::string& command)
{
  // TODO: Execute some python, or something :)
  LOG(INFO) << "exec: " << command;

  if (command == "imgui") {
    _showImGui = !_showImGui;
  }
}

bool Entity_scripting_manager::commandSuggestions(const std::string& command, std::vector<std::string>& suggestions)
{
  // TODO: provide some suggestions!
  return false;
}