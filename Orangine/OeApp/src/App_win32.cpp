#include "LogFileSink.h"
#include "VectorLogSink.h"
#include "VisualStudioLogSink.h"

#include <OeCore/OeCore.h>
#include <OeCore/StepTimer.h>
#include <OeCore/Perf_timer.h>
#include <OeCore/Entity_graph_loader_gltf.h>

#include <OeScripting/OeScripting.h>

#include <OePipelineD3D12/OePipelineD3D12.h>

#include <OeApp/App.h>
#include <OeApp/Manager_collection.h>
#include <OeApp/Yaml_config_reader.h>

#include <g3log/logworker.hpp>

#include <filesystem>
#include <wrl/wrappers/corewrappers.h>

const std::string path_to_log_file = "./";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
typedef void(__cdecl* GamePluginInitFn)(oe::Scene&);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

const auto CONFIG_FILE_NAME = L"config.yaml";

using oe::app::App;
namespace fs = std::filesystem;

#ifndef NTDDI_WIN10_FE
#error "Windows SDK 10.0.19041 must be installed and on the include path."
#else
static_assert(WDK_NTDDI_VERSION >= NTDDI_WIN10_FE, "Windows SDK must be at least 10.0.19041.");
#endif

namespace oe::app {
class App_impl {
 public:
  explicit App_impl(HWND hwnd)
      : _hwnd(hwnd)
      , _fatalError(false)
      , _deviceResources(nullptr)
  {
    _coreManagers = std::make_unique<oe::core::Manager_instances>();
    oe::core::for_each_manager_instance(_coreManagers->managers, [&](const Manager_interfaces& interfaces) {
      _managers.addManager(interfaces);
    });

    // Shows stuff in a window
    _pipelineManagers = std::make_unique<oe::pipeline_d3d12::Manager_instances>(*_coreManagers, hwnd);
    oe::core::for_each_manager_instance(_pipelineManagers->managers, [&](const Manager_interfaces& interfaces) {
      _managers.addManager(interfaces);
    });

    // Provides access to the engine via python
    _scriptingManagers = std::make_unique<oe::scripting::Manager_instances>(
            *_coreManagers, _pipelineManagers->getInstance<IInput_manager>(),
            _pipelineManagers->getInstance<IEntity_render_manager>());
    oe::core::for_each_manager_instance(_scriptingManagers->managers, [&](const Manager_interfaces& interfaces) {
      _managers.addManager(interfaces);
    });

    // Some helpful things.
    {
      auto devToolsManager = oe::create_manager_instance<IDev_tools_manager>(
              _coreManagers->getInstance<IScene_graph_manager>(),
              _pipelineManagers->getInstance<IEntity_render_manager>(),
              _pipelineManagers->getInstance<IMaterial_manager>(),
              _pipelineManagers->getInstance<IPrimitive_mesh_data_factory>());
      _devToolsManager = std::move(devToolsManager.instance);
    }
    _deviceResources = &_pipelineManagers->getInstance<IDevice_resources>();

    _managerTickTimes.resize(_managers.getTickableManagers().size());

    auto gltfLoader = std::make_unique<Entity_graph_loader_gltf>(
            _pipelineManagers->getInstance<IMaterial_manager>(),
            _pipelineManagers->getInstance<ITexture_manager>());
    _coreManagers->getInstance<IScene_graph_manager>().addLoader(std::move(gltfLoader));
  }
 public:

  void configureManagers() {
    try {
      _configReader = std::make_unique<Yaml_config_reader>(CONFIG_FILE_NAME);
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
          manager->loadConfig(*_configReader);
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
  void onTick() {
    _coreManagers->getInstance<ITime_step_manager>().progressTime(_stepTimer.GetElapsedSeconds());

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

    _pipelineManagers->getInstance<IRender_step_manager>().render();
    _pipelineManagers->getInstance<IUser_interface_manager>().render();
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

  HWND getWindow() const { return _hwnd; }

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
        LOG(DEBUG) << "Creating device dependent resources for " << manager.config->asBase->name();
        manager()->createDeviceDependentResources();
      };
    } catch (std::exception& e) {
      LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
      _fatalError = true;
    }
  }

  void createWindowSizeDependentResources() {
    try {
      _deviceResources->recreateWindowSizeDependentResources();
      int width = 0, height = 0;
      if (!_deviceResources->getWindowSize(width, height)) {
        LOG(WARNING) << "createWindowSizeDependentResources: Failed to get last window size.";
        return;
      }

      width = std::max(width, 1);
      height = std::max(height, 1);
      for (auto& manager : _managers.getWindowDependentManagers()) {
        LOG(DEBUG) << "Creating window size dependent resources for " << manager.config->asBase->name()
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
  IDevice_resources* _deviceResources;
  StepTimer _stepTimer;
  HWND const _hwnd;
  bool _fatalError;
  std::vector<double> _managerTickTimes;
  std::unique_ptr<core::Manager_instances> _coreManagers;
  std::unique_ptr<scripting::Manager_instances> _scriptingManagers;
  std::unique_ptr<pipeline_d3d12::Manager_instances> _pipelineManagers;
  std::vector<Manager_interfaces*> _initializedManagers;
  std::unique_ptr<Yaml_config_reader> _configReader;
  std::unique_ptr<IDev_tools_manager> _devToolsManager;
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

  // Turn path into absolute
  fs::path absLogFilePath(path_to_log_file);
  if (absLogFilePath.is_relative()) {
    absLogFilePath = fs::current_path() / absLogFilePath;
  }

  // Create a sink with a non-timebased filename
  auto fileLogSink =
      std::make_unique<oe::LogFileSink>(logFileName, absLogFilePath.lexically_normal().string(), "game", false);
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

  // Create the app context
  int progReturnVal = 1;
  _impl = new app::App_impl(hwnd);

  bool sceneConfigured = false;
  do {
    auto& appDeviceContext = *_impl;

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
          appDeviceContext._stepTimer.Tick([this, &appDeviceContext] {
            onScenePreTick();
            appDeviceContext.onTick();
          });

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

  LOG(INFO) << "Exited main loop";

  SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));

  if (sceneConfigured) {
    try {
      _impl->destroyWindowSizeDependentResources();
      _impl->destroyDeviceDependentResources();

      getDevToolsManager().setVectorLog(nullptr);

      onSceneShutdown();
      _impl->shutdownManagers();
    } catch (const std::exception& ex) {
      LOG(WARNING) << "Terminating on uncaught exception while shutting down: " << ex.what();
      progReturnVal = 1;
    } catch (...) {
      LOG(WARNING) << "Terminating on uncaught error while shutting down";
      progReturnVal = 1;
    }
  }

  delete _impl;

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

oe::IEntity_repository& App::getEntityRepository()
{
  return _impl->_coreManagers->getInstance<IEntity_repository>();
}
oe::ITime_step_manager& App::getTimeStepManager()
{
  return _impl->_coreManagers->getInstance<oe::ITime_step_manager>();
}
oe::IScene_graph_manager& App::getSceneGraphManager()
{
  return _impl->_coreManagers->getInstance<oe::IScene_graph_manager>();
}
oe::IAsset_manager& App::getAssetManager()
{
  return _impl->_coreManagers->getInstance<oe::IAsset_manager>();
}
oe::IUser_interface_manager& App::getUserInterfaceManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IUser_interface_manager>();
}
oe::ITexture_manager& App::getTextureManager()
{
  return _impl->_pipelineManagers->getInstance<oe::ITexture_manager>();
}
oe::IBehavior_manager& App::getBehaviorManager()
{
  return _impl->_coreManagers->getInstance<oe::IBehavior_manager>();
}
oe::IMaterial_manager& App::getMaterialManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IMaterial_manager>();
}
oe::ILighting_manager& App::getLightingManager()
{
  return _impl->_pipelineManagers->getInstance<oe::ILighting_manager>();
}
oe::IShadowmap_manager& App::getShadowmapManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IShadowmap_manager>();
}
oe::IInput_manager& App::getInputManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IInput_manager>();
}
oe::IAnimation_manager& App::getAnimationManager()
{
  return _impl->_coreManagers->getInstance<oe::IAnimation_manager>();
}
oe::IEntity_render_manager& App::getEntityRenderManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IEntity_render_manager>();
}
oe::IRender_step_manager& App::getRenderStepManager()
{
  return _impl->_pipelineManagers->getInstance<oe::IRender_step_manager>();
}
oe::IPrimitive_mesh_data_factory& App::getPrimitiveMeshDataFactory()
{
  return _impl->_pipelineManagers->getInstance<oe::IPrimitive_mesh_data_factory>();
}
oe::IEntity_scripting_manager& App::getEntityScriptingManager()
{
  return _impl->_scriptingManagers->getInstance<oe::IEntity_scripting_manager>();
}
oe::IDev_tools_manager& App::getDevToolsManager()
{
  return *_impl->_devToolsManager;
}
oe::IConfigReader& App::getConfigReader() {
  return *_impl->_configReader;
}
