#include "pch.h"

#include "Entity_scripting_manager.h"
#include "Script_runtime_data.h"

#include "OeCore/Renderer_types.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Entity.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/IInput_manager.h"
#include "OeCore/IScene_graph_manager.h"
#include "OeCore/Light_component.h"
#include "OeCore/Math_constants.h"
//#include "OeCore/Renderable_component.h"
#include "Engine_bindings.h"
#include "OeCore/Scene.h"
#include "OeCore/Test_component.h"
#include "OeCore/Shadow_map_texture.h"

#include <OeThirdParty/imgui.h>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace py = pybind11;

using namespace DirectX;
using namespace oe;
using namespace internal;

#ifdef OeScripting_PYTHON_DEBUG
#ifndef Py_DEBUG
// Well this was an evening wasted! When linking against a debug build of python, you must also set
// this flag in the consuming application.
#error Py_DEBUG must be defined if linking with debug python library.
#endif
#endif

// I don't know why this needs to be in this file, but it does.
// If I put it inside Engine_bindings.cpp it (sometimes) doesn't get called.
PYBIND11_EMBEDDED_MODULE(oe, m) {
  m.doc() = "Orangine scripting API";
  Engine_bindings::create(m);
}

Entity_scripting_manager::EngineInternalPythonModule::EngineInternalPythonModule(
    py::module engineInternal)
    : instance(engineInternal)
    , reset_output_streams(engineInternal.attr("reset_output_streams"))
    , enable_remote_debugging(engineInternal.attr("enable_remote_debugging")) {}

template <> IEntity_scripting_manager* oe::create_manager(Scene& scene) {
  return new Entity_scripting_manager(scene);
}

Entity_scripting_manager::Entity_scripting_manager(Scene& scene)
    : IEntity_scripting_manager(scene) {}

void Entity_scripting_manager::preInit_addAbsoluteScriptsPath(const std::wstring& path) {
  if (_pythonInitialized) {
    OE_THROW(std::logic_error(
        "preInit_addAbsoluteScriptsPath can only be called prior to manager initialization."));
  }
  if (!std::filesystem::exists(path)) {
    OE_THROW(std::runtime_error(
        "Attempting to add python script path that does not exist, or is inaccessible: " +
        utf8_encode(path)));
  }
  _preInit_additionalPaths.push_back(path);
}

void Entity_scripting_manager::initialize() {
  {
    _testEntityFilter =
        _scene.manager<IScene_graph_manager>().getEntityFilter({Test_component::type()});

    // TODO: can't inlclude renderable_component right now
    /*
    _renderableEntityFilter =
        _scene.manager<IScene_graph_manager>().getEntityFilter({Renderable_component::type()});
        */
    _lightEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter(
        {Directional_light_component::type(),
         Point_light_component::type(),
         Ambient_light_component::type()},
        Entity_filter_mode::Any);

    try {
      initializePythonInterpreter();
      return;
    } catch (const py::error_already_set& err) {
      logPythonError(err, "service initialization");
    }
  }

  // This must happen outside of a catch, so that the error_already_set object is destructed prior
  // to throwing.
  finalizePythonInterpreter();
  throw ::std::runtime_error("Failed to initialize Entity Scripting Manager.");
}

void Entity_scripting_manager::logPythonError(
    const py::error_already_set& err,
    const std::string& whereStr) {
  std::stringstream ss;
  ss << "Python error in " << whereStr << ": " << std::string(py::str(err.value()));

  auto traceback = err.trace();
  while (!traceback.is_none() && traceback.ptr() != nullptr) {
    std::string lineno = std::string(py::str(traceback.attr("tb_lineno")));
    std::string filename =
        std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_filename")));
    std::string funcname =
        std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_name")));

    ss << std::endl;
    ss << " at " << filename << ":" << lineno << " -> " << funcname;

    traceback = traceback.attr("tb_next");
  }

  LOG(WARNING) << ss.str();
}

void Entity_scripting_manager::tick() {
  for (auto& runtimeDataPtr : _loadedScripts) {
    assert(runtimeDataPtr.get());
    auto& runtimeData = *runtimeDataPtr;

    if (!runtimeData.hasTick)
      continue;

    try {
      auto _ = runtimeData.instance.attr("tick")();
    } catch (const py::error_already_set& err) {
      logPythonError(err, "Native::tick() " + runtimeData.scriptName);
    }
  }

  try {
    flushStdIo();
  } catch (const py::error_already_set& err) {
    logPythonError(err, "Native::tick() - Flushing stdout and stderr");
  }

  const auto elapsedTime = _scene.elapsedTime();
  for (auto iter = _testEntityFilter->begin(); iter != _testEntityFilter->end(); ++iter) {
    auto& entity = **iter;
    auto* component = entity.getFirstComponentOfType<Test_component>();
    if (component == nullptr)
      continue;

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
              _scriptData.distance + static_cast<float>(mouseState->scrollWheelDelta) * -mouseSpeed));
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

void Entity_scripting_manager::loadSceneScript(const std::string& scriptClassString) {

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
    } else {
      const auto scriptClassDefn = py::globals().attr(scriptClassString.c_str());

      runtimeDataPtr->instance = scriptClassDefn();
    }

    runtimeDataPtr->hasInit = py::hasattr(runtimeDataPtr->instance, "init");
    runtimeDataPtr->hasTick = py::hasattr(runtimeDataPtr->instance, "tick");

    _loadedScripts.emplace_back(std::move(runtimeDataPtr));
  } catch (const py::error_already_set& err) {
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
  } catch (const py::error_already_set& err) {
    logPythonError(err, "Native::loadSceneScript init - " + scriptClassString);
  }
}


void Entity_scripting_manager::flushStdIo() const {
  const auto _ = _pythonContext._engineInternalModule->reset_output_streams();
}

void Entity_scripting_manager::enableRemoteDebugging() const {
  const auto _ = _pythonContext._engineInternalModule->enable_remote_debugging();
}

void Entity_scripting_manager::shutdown() { finalizePythonInterpreter(); }

void Entity_scripting_manager::initializePythonInterpreter() {
  py::initialize_interpreter();

  _pythonInitialized = true;

  std::vector<std::wstring> sysPathVec;

  const auto& dataLibPath =
      _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(L"OeScripting/lib");
  sysPathVec.push_back(dataLibPath);

  const auto& pyEnvPath = _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(L"pyenv") + L"/..";

  // Add pyenv scripts
  sysPathVec.push_back(pyEnvPath + L"/lib");
  sysPathVec.push_back(pyEnvPath + L"/lib/site-packages");

  // Add system installed python (for non-installed, dev machine builds only). Must come after pyenv
  auto pyenvCfgPath = pyEnvPath + L"/pyvenv.cfg";
  if (std::filesystem::exists(pyenvCfgPath)) {
    std::string line;
    std::ifstream infile(pyenvCfgPath, std::ios::in);
    while (std::getline(infile, line)) {
      auto equalPos = line.find_first_of('=');
      if (std::string::npos == equalPos) {
        LOG(DEBUG) << "Failed to parse line in " << utf8_encode(pyenvCfgPath) << ": " << line;
        continue;
      }

      std::string test1 = str_trim("   hello");
      std::string test2 = str_trim("   hello   ");
      std::string test3 = str_trim("hello   ");
      std::string test4 = str_trim("");

      std::string propName = str_trim(line.substr(0, equalPos - 1));
      std::string value = str_trim(line.substr(equalPos + 1));

      if (propName == "home") {
        LOG(DEBUG) << "Detected python home from " << utf8_encode(pyenvCfgPath) << ": " << value;
        sysPathVec.push_back(utf8_decode(value) + L"/Lib");
        sysPathVec.push_back(utf8_decode(value) + L"/DLLs");
      }
      if (propName == "include-system-site-packages" && value == "true") {
        LOG(DEBUG) << "Detected include-system-site-packages from " << utf8_encode(pyenvCfgPath);
        sysPathVec.push_back(utf8_decode(value) + L"/Lib/site-packages");
      }
    }
  }

  // Add OeScripting scripts
  const auto& dataScriptsPath =
      _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(L"OeScripting/scripts");
  sysPathVec.push_back(dataScriptsPath);

  // Add additional user script paths
  std::copy(
      _preInit_additionalPaths.begin(),
      _preInit_additionalPaths.end(),
      std::back_inserter(sysPathVec));

  // Convert to a python list, and overwrite the existing value of sys.path
  auto sysPathList = py::list();
  std::stringstream sysPathListSs;
  for (int i = 0, e = static_cast<int>(sysPathVec.size()); i < e; ++i) {
    const auto path = std::filesystem::absolute(utf8_encode(sysPathVec[i])).u8string();
    sysPathList.append(path);
    if (i)
      sysPathListSs << ";";
    sysPathListSs << path;
  }

  LOG(INFO) << "Python path set to: " << sysPathListSs.str();

  _pythonContext._sysModule = py::module::import("sys");
  _pythonContext._sysModule.attr("path") = sysPathList;

  _pythonContext._engineInternalModule =
      std::make_unique<EngineInternalPythonModule>(py::module::import("engine_internal"));

  const auto scenePtr = &_scene;
#ifdef _AMD64_
  _pythonContext._engineInternalModule->instance.attr("scenePtr_uint64") =
      reinterpret_cast<uint64_t>(scenePtr);
#elif _X86_
  _pythonContext.engine_internal_module->instance.attr("scenePtr_uint32") =
      reinterpret_cast<uint32_t>(scenePtr);
#endif  

  // This must be called at least once before python attempts to write to stdout or stderr.
  // Now that we have our internal library, set up stdout and stderr properly.
  flushStdIo();

  enableRemoteDebugging();

  auto initFn = _pythonContext._engineInternalModule->instance.attr("init");
  auto _ = initFn();
}

void Entity_scripting_manager::finalizePythonInterpreter() {

  _loadedScripts.clear();
  _pythonContext = {};

  if (_pythonInitialized) {
    // This must happen last, after all our handles to python objects are cleared.
    py::finalize_interpreter();
    _pythonInitialized = false;
  }
}

void Entity_scripting_manager::renderImGui() {
  if (_showImGui) {
    ImGui::ShowDemoWindow();
  }
}

void Entity_scripting_manager::execute(const std::string& command) {
  // TODO: Execute some python, or something :)
  LOG(INFO) << "exec: " << command;

  if (command == "imgui") {
    _showImGui = !_showImGui;
  }
}

bool Entity_scripting_manager::commandSuggestions(
    const std::string& command,
    std::vector<std::string>& suggestions) {
  // TODO: provide some suggestions!
  return false;
}

void Entity_scripting_manager::renderDebugSpheres() const {
  auto& renderManager = _scene.manager<IEntity_render_manager>();
  auto& devToolsManager = _scene.manager<IDev_tools_manager>();
  devToolsManager.clearDebugShapes();

  if (_renderableEntityFilter != nullptr) {
    for (const auto& entity : *_renderableEntityFilter) {
      const auto& boundSphere = entity->boundSphere();
      const auto transform =
          entity->worldTransform() * SSE::Matrix4::translation(boundSphere.center);
      devToolsManager.addDebugSphere(transform, boundSphere.radius, Colors::Gray);
    }
  }

  const auto mainCameraEntity = _scene.mainCamera();
  if (mainCameraEntity != nullptr) {
    const auto cameraComponent = mainCameraEntity->getFirstComponentOfType<Camera_component>();
    if (cameraComponent) {
      auto frustum = renderManager.createFrustum(*cameraComponent);
      frustum.farPlane *= 0.5;
      devToolsManager.addDebugFrustum(frustum, Colors::Red);
    }
  }

  for (const auto& entity : *_lightEntityFilter) {
    const auto directionalLight = entity->getFirstComponentOfType<Directional_light_component>();
    if (directionalLight && directionalLight->shadowsEnabled()) {
      const auto& shadowData = directionalLight->shadowData();
      if (shadowData) {
        devToolsManager.addDebugBoundingBox(shadowData->boundingOrientedBox, directionalLight->color());
      }
    }
  }
}
