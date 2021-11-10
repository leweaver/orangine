//
// Main.cpp
//

#include "pch.h"

#include "LogFileSink.h"
#include "VectorLogSink.h"
#include "VisualStudioLogSink.h"

#include <OeCore/Perf_timer.h>
#include <OeCore/Entity_repository.h>
#include <OeCore/JsonConfigReader.h>
#include <OeCore/Entity_graph_loader_gltf.h>

#include <OeApp/App.h>
#include <OeApp/Manager_collection.h>

#include <g3log/logworker.hpp>

#include <filesystem>
#include <wrl/wrappers/corewrappers.h>

#include <OeCore/StepTimer.h>
#include <json.hpp>

const std::string path_to_log_file = "./";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
typedef void(__cdecl* GamePluginInitFn)(oe::Scene&);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

extern std::unique_ptr<oe::IDevice_resources> createWin32DeviceResources(HWND hwnd);

const auto CONFIG_FILE_NAME = L"config.json";

using oe::app::App;

namespace oe::app {
class App_impl {
 public:
  App_impl(
      HWND hwnd,
      IDevice_resources* deviceResources)
      : _deviceResources(deviceResources)
      , _hwnd(hwnd)
      , _fatalError(false) {
    createManagers();
  }

 private:
  void createManagers() {
    _entityRepository = std::make_shared<Entity_repository>();
    auto entityRepository = std::static_pointer_cast<IEntity_repository>(_entityRepository);

    auto timeStepManager = create_manager_instance<ITime_step_manager>(static_cast<const StepTimer&>(_stepTimer));
    auto sceneGraphManager = create_manager_instance<IScene_graph_manager>(entityRepository);
    auto assetManager = create_manager_instance<IAsset_manager>();

    // Only device dependencies
    auto& deviceResources = *_deviceResources;
    auto userInterfaceManager = create_manager_instance<IUser_interface_manager>(deviceResources);
    auto textureManager = create_manager_instance<ITexture_manager>(deviceResources);

    auto behaviorManager = create_manager_instance<IBehavior_manager>(*sceneGraphManager.instance);
    auto materialManager = create_manager_instance<IMaterial_manager>(*assetManager.instance);
    auto lightingManager = create_manager_instance<ILighting_manager>(*textureManager.instance);
    auto shadowmapManager = create_manager_instance<IShadowmap_manager>(*textureManager.instance);
    auto inputManager = create_manager_instance<IInput_manager>(*userInterfaceManager.instance);

    auto animationManager = create_manager_instance<IAnimation_manager>(*sceneGraphManager.instance, *timeStepManager.instance);
    auto entityRenderManager = create_manager_instance<IEntity_render_manager>(
            *textureManager.instance, *materialManager.instance, *lightingManager.instance);

    // Provides access to the engine via python
    auto entityScriptingManager = create_manager_instance<IEntity_scripting_manager>(
            *timeStepManager.instance, *sceneGraphManager.instance, *inputManager.instance, *assetManager.instance, *entityRenderManager.instance);

    auto devToolsManager = create_manager_instance<IDev_tools_manager>(
            *sceneGraphManager.instance, *entityRenderManager.instance, *materialManager.instance,
            *entityScriptingManager.instance);

    // Pulls everything together and draws pixels
    auto renderStepManager = create_manager_instance<IRender_step_manager>(*sceneGraphManager.instance, *devToolsManager.instance,
                                        *textureManager.instance, *shadowmapManager.instance,
                                        *entityRenderManager.instance, *lightingManager.instance);

    // Don't assign these above just to check that dependencies are created in the right order at compile time.
    _timeStepManager = std::move(timeStepManager), _managers.addManager(_timeStepManager.interfaces);
    _sceneGraphManager = std::move(sceneGraphManager), _managers.addManager(_sceneGraphManager.interfaces);
    _assetManager = std::move(assetManager), _managers.addManager(_assetManager.interfaces);
    _userInterfaceManager = std::move(userInterfaceManager), _managers.addManager(_userInterfaceManager.interfaces);
    _textureManager = std::move(textureManager), _managers.addManager(_textureManager.interfaces);
    _behaviorManager = std::move(behaviorManager), _managers.addManager(_behaviorManager.interfaces);
    _materialManager = std::move(materialManager), _managers.addManager(_materialManager.interfaces);
    _lightingManager = std::move(lightingManager), _managers.addManager(_lightingManager.interfaces);
    _shadowmapManager = std::move(shadowmapManager), _managers.addManager(_shadowmapManager.interfaces);
    _inputManager = std::move(inputManager), _managers.addManager(_inputManager.interfaces);
    _animationManager = std::move(animationManager), _managers.addManager(_animationManager.interfaces);
    _entityRenderManager = std::move(entityRenderManager), _managers.addManager(_entityRenderManager.interfaces);
    _devToolsManager = std::move(devToolsManager), _managers.addManager(_devToolsManager.interfaces);
    _renderStepManager = std::move(renderStepManager), _managers.addManager(_renderStepManager.interfaces);
    _entityScriptingManager = std::move(entityScriptingManager), _managers.addManager(_entityScriptingManager.interfaces);

    _managerTickTimes.resize(_managers.getTickableManagers().size());

    auto gltfLoader = std::make_unique<Entity_graph_loader_gltf>(*_materialManager.instance, *_textureManager.instance);
    _sceneGraphManager.instance->addLoader(std::move(gltfLoader));
  }
 public:

  void configureManagers() {
    std::unique_ptr<JsonConfigReader> configReader;
    try {
      configReader = std::make_unique<JsonConfigReader>(CONFIG_FILE_NAME);
    }
    catch (std::exception& ex) {
      LOG(WARNING) << "Failed to read config file: " << utf8_encode(CONFIG_FILE_NAME) << "(" << ex.what() << ")";
      OE_THROW(std::runtime_error(std::string("Scene init failed reading config. ") + ex.what()));
    }

    try {
      for (auto& managerInterfaces : _managers.getAllManagers()) {
        auto manager = managerInterfaces->asBase;
        if (manager == nullptr) {return;}

        try {
          manager->loadConfig(*configReader);
        }
        catch (std::exception& ex) {
          OE_THROW(std::runtime_error("Failed to configure " + manager->name() + ": " + ex.what()));
        }
      };
    }
    catch (std::exception& ex) {
      LOG(WARNING) << "Failed to configure managers: " << ex.what();
      OE_THROW(std::runtime_error(std::string("Scene configure failed. ") + ex.what()));
    }
  }

  void initializeManagers() {

    try {
      for (auto& managerInterfaces : _managers.getAllManagers()) {
        auto manager = managerInterfaces->asBase;
        if (manager == nullptr) {
          continue;
        }

        try {
          manager->initialize();
        }
        catch (std::exception& ex) {
          OE_THROW(std::runtime_error("Failed to initialize " + manager->name() + ": " + ex.what()));
        }

        _initializedManagers.push_back(managerInterfaces.get());
        managerInterfaces->asBase->initialize();
      }
    }
    catch (std::exception& ex) {
      LOG(WARNING) << "Failed to initialize managers: " << ex.what();
      OE_THROW(std::runtime_error(std::string("Scene init failed. ") + ex.what()));
    }
  }

  void shutdownManagers() {

    std::stringstream ss;
    const double tickCount = _stepTimer.GetFrameCount();

    if (tickCount > 0) {
      const auto& tickableManagers = _managers.getTickableManagers();
      for (auto i = 0U; i < tickableManagers.size(); ++i) {
        const auto& mgrConfig = tickableManagers.at(i);

        for (int idx = 0; idx < tickableManagers.size(); idx++) {
          auto manager = tickableManagers.at(idx);
          if (_managerTickTimes.at(idx) > 0.0) {
            ss << "  " << manager.config->asBase->name() << ": " << (1000.0 * _managerTickTimes.at(idx) / tickCount)
               << std::endl;
          }
        }
        LOG(INFO) << "Manager average tick times (ms): " << std::endl << ss.str();
      }
    }

    std::for_each(_initializedManagers.rbegin(), _initializedManagers.rend(), [](const auto& managerInterfaces) {
      managerInterfaces->asBase->shutdown();
    });
  }

  // Game Loop
  HWND getWindow() const { return _hwnd; }
  void onTick() {
    auto perfTimer = Perf_timer();
    const auto& tickableManagers = _managers.getTickableManagers();
    for (auto i = 0U; i < tickableManagers.size(); ++i) {
      const auto& mgrConfig = tickableManagers.at(i);
      perfTimer.restart();
      mgrConfig()->tick();
      perfTimer.stop();
      _managerTickTimes.at(i) += perfTimer.elapsedSeconds();
    }

    // Hack to reset timers after first frame which is typically quite heavy.
    if (_stepTimer.GetFrameCount() == 1) {
      std::fill(_managerTickTimes.begin(), _managerTickTimes.end(), 0.0);
    }
  }

  void onRender() {
    // Don't try to render anything before the first Update.
    if (_stepTimer.GetFrameCount() == 0) {
      return;
    }

    _renderStepManager.instance->render();
    _userInterfaceManager.instance->render();
  }

  // Messages
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) const {
    bool handled = false;
    for (auto& manager : _managers.getWindowsMessageProcessorManagers()) {
      handled = manager()->processMessage(message, wParam, lParam);
      if (handled) {
        break;
      }
    }

    return handled;
  }

  void onActivated() {
    // TODO: Game is becoming active window.
  }
  void onDeactivated() {
    // TODO: Game is becoming background window.
  }
  void onSuspending() {
    // Game is being power-suspended (or minimized).
  }
  void onResuming() {
    // Game is being power-resumed (or returning from minimize).
    _stepTimer.ResetElapsedTime();

  }
  void onWindowMoved() {
    int width = 0, height = 0;
    if (!_deviceResources->getWindowSize(width, height)) {
      LOG(WARNING) << "onWindowMoved: Failed to get last window size.";
      return;
    }
    _deviceResources->setWindowSize(width, height);
  }
  void onWindowSizeChanged(int width, int height) {
    int oldWidth = width, oldHeight = height;
    if (!_deviceResources->getWindowSize(oldWidth, oldHeight)) {
      LOG(WARNING) << "onWindowSizeChanged: Failed to get last window size.";
      return;
    }

    if (oldWidth == width && oldHeight == height) {
      LOG(INFO) << "Window size unchanged, ignoring event: " << width << ", " << height;
      return;
    }

    LOG(INFO) << "Window size changed: " << width << ", " << height;
    destroyWindowSizeDependentResources();

    if (!_deviceResources->setWindowSize(width, height)) {
      LOG(WARNING) << "Device resources failed to handle WindowSizeChanged event.";
    }

    createWindowSizeDependentResources();
  }

  // Properties
  void createDeviceDependentResources() {
    try {
      _deviceResources->createDeviceDependentResources();

      for (auto& manager : _managers.getDeviceDependentManagers()) {
        LOG(DEBUG) << "Creating device dependent resources for manager " << manager.config->asBase->name();
        manager()->createDeviceDependentResources();
      };
    } catch (std::exception& e) {
      LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
      _fatalError = true;
    }
  }

  void createWindowSizeDependentResources() {
    try {
      int width = 0, height = 0;
      if (!_deviceResources->getWindowSize(width, height)) {
        LOG(WARNING) << "createWindowSizeDependentResources: Failed to get last window size.";
        return;
      }

      width = std::max(width, 1);
      height = std::max(height, 1);
      for (auto& manager : _managers.getWindowDependentManagers()) {
        LOG(DEBUG) << "Creating device dependent resources for manager " << manager.config->asBase->name()
                   << " (width=" << width << " height=" << height << ")";
        manager()->createWindowSizeDependentResources(_hwnd, width, height);
      };
    } catch (std::exception& e) {
      LOG(FATAL) << "Failed to create window size dependent resources: " << e.what();
      _fatalError = true;
    }
  }

  void destroyWindowSizeDependentResources() {
    LOG(INFO) << "Destroying window size dependent resources";
    for (auto& manager : _managers.getWindowDependentManagers()) {
      manager()->destroyWindowSizeDependentResources();
    };
  }

  void destroyDeviceDependentResources() {

    LOG(INFO) << "Destroying device dependent resources";
    for (auto& manager : _managers.getDeviceDependentManagers()) {
      manager()->destroyDeviceDependentResources();
    };
    _deviceResources->destroyDeviceDependentResources();
  }

  bool hasFatalError() const { return _fatalError; }
  static bool createWindow(const App_start_settings& settings, HWND& hwnd) {
    // TODO: We should try and get the hInstance that was passed to WinMain somehow.
    // BUT... using the HINST_THISCOMPONENT macro defined above seems to break 64bit builds.
    HINSTANCE hInstance = 0;

    // Register class and create window
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, L"IDI_ICON");
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"OeAppWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, L"IDI_ICON");
    if (!RegisterClassEx(&wcex)) {
      LOG(WARNING) << "Failed to register window class: " << getlasterror_to_str();
      return 1;
    }

    // Create window
    int w = 0, h = 0;
    getDefaultWindowSize(w, h);

    RECT rc;
    rc.top = 0;
    rc.left = 0;
    rc.right = static_cast<LONG>(w);
    rc.bottom = static_cast<LONG>(h);

    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    auto dwExStyle = settings.fullScreen ? WS_EX_TOPMOST : 0;
    auto dwStyle = settings.fullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    hwnd = CreateWindowEx(
        dwExStyle,
        L"OeAppWindowClass",
        settings.title.c_str(),
        dwStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd) {
      LOG(WARNING) << "Failed to create window: " << getlasterror_to_str();
      return false;
    }

    ShowWindow(hwnd, settings.fullScreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);

    return true;
  }

  static void getDefaultWindowSize(int& w, int& h) {

    // Get dimensions of the primary monitor
    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);

    // Set window to 75% of the desktop size
    w = std::max(320l, (rc.right * 3) / 4);
    h = std::max(200l, (rc.bottom * 3) / 4);
  }

 private:
  friend App;

  Manager_collection _managers;
  IDevice_resources* const _deviceResources;
  StepTimer _stepTimer;
  HWND const _hwnd;
  bool _fatalError;
  std::vector<double> _managerTickTimes;

  std::shared_ptr<Entity_repository> _entityRepository;
  Manager_instance<ITime_step_manager> _timeStepManager;
  Manager_instance<IScene_graph_manager> _sceneGraphManager;
  Manager_instance<IAsset_manager> _assetManager;
  Manager_instance<IUser_interface_manager> _userInterfaceManager;
  Manager_instance<ITexture_manager> _textureManager;
  Manager_instance<IBehavior_manager> _behaviorManager;
  Manager_instance<IMaterial_manager> _materialManager;
  Manager_instance<ILighting_manager> _lightingManager;
  Manager_instance<IShadowmap_manager> _shadowmapManager;
  Manager_instance<IInput_manager> _inputManager;
  Manager_instance<IAnimation_manager> _animationManager;
  Manager_instance<IEntity_render_manager> _entityRenderManager;
  Manager_instance<IDev_tools_manager> _devToolsManager;
  Manager_instance<IRender_step_manager> _renderStepManager;
  Manager_instance<IEntity_scripting_manager> _entityScriptingManager;

  std::vector<Manager_interfaces*> _initializedManagers;
};
} // namespace oe

// Do NOT do this in 64 bit builds, it is incompatible with json.hpp for some reason.
// EXTERN_C IMAGE_DOS_HEADER __ImageBase;
//#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Entry point
int App::run(const App_start_settings& settings) {
  // Initialize logger
  std::string logFileName;
  auto logWorker = g3::LogWorker::createLogWorker();
  {
    auto execName = oe::utf8_encode(__wargv[0]);
    auto pos = execName.find_last_of('\\');
    if (pos != std::string::npos)
      logFileName = execName.substr(pos + 1);
    else
      logFileName = execName;
    execName = oe::str_replace_all(execName, "\\", "/");
  }

  // Create a sink with a non-timebased filename
  auto fileLogSink =
      std::make_unique<oe::LogFileSink>(logFileName, path_to_log_file, "game", false);
  fileLogSink->fileInit();
  auto fileLogSinkHandle = logWorker->addSink(move(fileLogSink), &oe::LogFileSink::fileWrite);

  auto vectorLog = std::make_shared<oe::VectorLog>(100);
  auto vectorLogSinkHandle = logWorker->addSink(
      move(std::make_unique<oe::VectorLogSink>(vectorLog)), &oe::VectorLogSink::append);

  auto visualStudioLogSinkHandle = logWorker->addSink(
      move(std::make_unique<oe::VisualStudioLogSink>()), &oe::VisualStudioLogSink::append);

  g3::initializeLogging(logWorker.get());

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
  Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
  if (FAILED(initialize)) {
    LOG(WARNING) << "Failed to initialize Win32: " << getlasterror_to_str();
    return 1;
  }
#else
  HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed to initialize Win32: " << getlasterror_to_str();
    return 1;
  }
#endif

  HWND hwnd;
  if (!App_impl::createWindow(settings, hwnd)) {
    LOG(WARNING) << "Failed to create window";
    return 1;
  }

  // Try to create a rendering device
  auto deviceResources = createWin32DeviceResources(hwnd);
  if (!deviceResources || !deviceResources->checkSystemSupport(true)) {
    return 1;
  }

  // Create the app context
  int progReturnVal = 1;
  auto appDeviceContext = app::App_impl(hwnd, deviceResources.get());
  _impl = &appDeviceContext;

  bool sceneConfigured = false;
  do {
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    try {

      appDeviceContext.configureManagers();
      sceneConfigured = true;

      getDevToolsManager().setVectorLog(vectorLog.get());
      onSceneConfigured();
    } catch (const std::exception& e) {
      LOG(WARNING) << "Failed to configure scene: " << e.what();
      progReturnVal = 1;
      break;
    }

    try {
      // Some things need setting before initialization.
      getUserInterfaceManager().preInit_setUIScale(static_cast<float>(getScreenDpi()) / 96.0f);

      appDeviceContext.initializeManagers();

      onSceneInitialized();
    } catch (std::exception& e) {
      LOG(WARNING) << "Failed to init scene: " << e.what();
      progReturnVal = 1;
      break;
    }

    // Create device resources
    try {
      appDeviceContext.createDeviceDependentResources();
      appDeviceContext.createWindowSizeDependentResources();
    } catch (const std::exception& e) {
      LOG(WARNING) << "Failed to create device resources: " << e.what();
      progReturnVal = 1;
      break;
    }

    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message && !appDeviceContext.hasFatalError()) {
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        progReturnVal = (int)msg.wParam;
      } else {
        try {
          onScenePreTick();
          appDeviceContext.onTick();

          onScenePreRender();
          appDeviceContext.onRender();
        } catch (const std::exception& ex) {
          LOG(WARNING) << "Terminating on uncaught exception: " << ex.what();
          progReturnVal = 1;
          break;
        } catch (...) {
          LOG(WARNING) << "Terminating on uncaught error";
          progReturnVal = 1;
          break;
        }
      }
    }
  } while (false);

  _impl = nullptr;
  SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));

  if (sceneConfigured) {
    try {
      appDeviceContext.destroyWindowSizeDependentResources();
      appDeviceContext.destroyDeviceDependentResources();

      getDevToolsManager().setVectorLog(nullptr);

      onSceneShutdown();
      appDeviceContext.shutdownManagers();
    } catch (const std::exception& ex) {
      LOG(WARNING) << "Terminating on uncaught exception while shutting down: " << ex.what();
      progReturnVal = 1;
    } catch (...) {
      LOG(WARNING) << "Terminating on uncaught error while shutting down";
      progReturnVal = 1;
    }
  }

  // This must be the last thing that happens.
  CoUninitialize();

  return progReturnVal;
}

void App::getDefaultWindowSize(int& w, int& h) const { _impl->getDefaultWindowSize(w, h); }

// Get DPI of the screen that the current window is on
int App::getScreenDpi() const {
  if (_impl == nullptr) {
    OE_THROW(std::runtime_error("App is not initialized"));
  }
  HWND window = _impl->getWindow();
  if (window == nullptr) {
    OE_THROW(std::runtime_error("App window not valid"));
  }

  return GetDpiForWindow(window);
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  PAINTSTRUCT ps;
  HDC hdc;

  static bool s_in_sizemove = false;
  static bool s_in_suspend = false;
  static bool s_minimized = false;
  static bool s_fullscreen = false;
  // TODO: Set s_fullscreen to true if defaulting to fullscreen.

  auto* app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  oe::app::App_impl* appImpl = nullptr;
  if (app != nullptr) {
    appImpl = app->getAppImpl();
  }

  // Give the game managers a change to process messages (for mouse input etc)
  if (appImpl != nullptr) {
    if (appImpl->processMessage(message, wParam, lParam)) {
      return true;
    }
  }

  switch (message) {
  case WM_PAINT:
    if (s_in_sizemove && appImpl) {
      appImpl->onTick();
    } else {
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
    }
    break;

  case WM_MOVE:
    if (appImpl) {
      appImpl->onWindowMoved();
    }
    break;

  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) {
      if (!s_minimized) {
        s_minimized = true;
        if (!s_in_suspend && appImpl)
          appImpl->onSuspending();
        s_in_suspend = true;
      }
    } else if (s_minimized) {
      s_minimized = false;
      if (s_in_suspend && appImpl)
        appImpl->onResuming();
      s_in_suspend = false;
    } else if (!s_in_sizemove && appImpl) {
      appImpl->onWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
    }
    break;

  case WM_ENTERSIZEMOVE:
    s_in_sizemove = true;
    break;

  case WM_EXITSIZEMOVE:
    s_in_sizemove = false;
    if (appImpl) {
      RECT rc;
      GetClientRect(hWnd, &rc);

      appImpl->onWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
    }
    break;

  case WM_GETMINMAXINFO: {
    auto info = reinterpret_cast<MINMAXINFO*>(lParam);
    info->ptMinTrackSize.x = 320;
    info->ptMinTrackSize.y = 200;
  } break;

  case WM_ACTIVATEAPP:
    if (appImpl) {
      if (wParam) {
        appImpl->onActivated();
      } else {
        appImpl->onDeactivated();
      }
    }
    break;

  case WM_POWERBROADCAST:
    switch (wParam) {
    case PBT_APMQUERYSUSPEND:
      if (!s_in_suspend && appImpl)
        appImpl->onSuspending();
      s_in_suspend = true;
      return TRUE;

    case PBT_APMRESUMESUSPEND:
      if (!s_minimized) {
        if (s_in_suspend && appImpl)
          appImpl->onResuming();
        s_in_suspend = false;
      }
      return TRUE;
    }
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_SYSKEYDOWN:
    if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000) {
      // Implements the classic ALT+ENTER fullscreen toggle
      if (s_fullscreen) {
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

        auto width = 800;
        auto height = 600;
        if (app)
          app->getDefaultWindowSize(width, height);

        ShowWindow(hWnd, SW_SHOWNORMAL);

        SetWindowPos(
            hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
      } else {
        SetWindowLongPtr(hWnd, GWL_STYLE, 0);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

        SetWindowPos(
            hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
      }

      s_fullscreen = !s_fullscreen;
    }
    break;

  case WM_MENUCHAR:
    // A menu is active and the user presses a key that does not correspond
    // to any mnemonic or accelerator key. Ignore so we don't produce an error
    // beep.
    return MAKELRESULT(0, MNC_CLOSE);
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame()
{
  PostQuitMessage(0);
}

oe::Entity_repository& App::getEntityRepository()
{
  return *_impl->_entityRepository;
}
oe::ITime_step_manager& App::getTimeStepManager()
{
  return *_impl->_timeStepManager.instance;
}
oe::IScene_graph_manager& App::getSceneGraphManager()
{
  return *_impl->_sceneGraphManager.instance;
}
oe::IAsset_manager& App::getAssetManager()
{
  return *_impl->_assetManager.instance;
}
oe::IUser_interface_manager& App::getUserInterfaceManager()
{
  return *_impl->_userInterfaceManager.instance;
}
oe::ITexture_manager& App::getTextureManager()
{
  return *_impl->_textureManager.instance;
}
oe::IBehavior_manager& App::getBehaviorManager()
{
  return *_impl->_behaviorManager.instance;
}
oe::IMaterial_manager& App::getMaterialManager()
{
  return *_impl->_materialManager.instance;
}
oe::ILighting_manager& App::getLightingManager()
{
  return *_impl->_lightingManager.instance;
}
oe::IShadowmap_manager& App::getShadowmapManager()
{
  return *_impl->_shadowmapManager.instance;
}
oe::IInput_manager& App::getInputManager()
{
  return *_impl->_inputManager.instance;
}
oe::IAnimation_manager& App::getAnimationManager()
{
  return *_impl->_animationManager.instance;
}
oe::IEntity_render_manager& App::getEntityRenderManager()
{
  return *_impl->_entityRenderManager.instance;
}
oe::IDev_tools_manager& App::getDevToolsManager()
{
  return *_impl->_devToolsManager.instance;
}
oe::IRender_step_manager& App::getRenderStepManager()
{
  return *_impl->_renderStepManager.instance;
}
oe::IEntity_scripting_manager& App::getEntityScriptingManager()
{
  return *_impl->_entityScriptingManager.instance;
}