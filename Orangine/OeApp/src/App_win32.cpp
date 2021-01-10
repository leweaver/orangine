//
// Main.cpp
//

#include "pch.h"

#include "LogFileSink.h"
#include "VectorLogSink.h"
#include "VisualStudioLogSink.h"

#include "OeCore/Scene.h"
#include <OeApp/App.h>
#include <OeCore/EngineUtils.h>
#include <OeCore/WindowsDefines.h>

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

extern std::unique_ptr<oe::IDevice_resources> createWin32DeviceResources(HWND hwnd);

namespace oe {
class App_impl {
 public:
  App_impl(
      App* app,
      HWND hwnd,
      IDevice_resources* deviceResources,
      Scene_device_resource_aware* scene)
      : _app(app)
      , _deviceResources(deviceResources)
      , _scene(scene)
      , _hwnd(hwnd)
      , _fatalError(false) {}

  // Game Loop
  HWND getWindow() const { return _hwnd; }
  void onTick() {

    _timer.Tick([this]() { _scene->tick(_timer); });
  }
  void onRender() {

    // Don't try to render anything before the first Update.
    if (_timer.GetFrameCount() == 0) {
      return;
    }

    _scene->manager<IRender_step_manager>().render(_scene->getMainCamera());
    _scene->manager<IUser_interface_manager>().render();
  }

  // Messages
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) const {
    return _scene->processMessage(message, wParam, lParam);
  }
  void onActivated() {
    // TODO: Game is becoming active window.
  }
  void onDeactivated() {
    // TODO: Game is becoming background window.
  }
  void onSuspending() {
    // TODO: Game is being power-suspended (or minimized).
  }
  void onResuming() {
    _timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
  }
  void onWindowMoved() {
    int width, height;
    if (!_deviceResources->getWindowSize(width, height)) {
      LOG(WARNING) << "onWindowMoved: Failed to get last window size.";
      return;
    }
    _deviceResources->setWindowSize(width, height);
  }
  void onWindowSizeChanged(int width, int height) {
    int oldWidth, oldHeight;
    if (!_deviceResources->getWindowSize(oldWidth, oldHeight)) {
      LOG(WARNING) << "onWindowSizeChanged: Failed to get last window size.";
      return;
    }

    if (oldWidth == width && oldHeight == height) {
      LOG(INFO) << "Window size unchanged, ignoring event: " << width << ", " << height;
      return;
    }

    LOG(INFO) << "Window size changed: " << width << ", " << height;
    _scene->destroyWindowSizeDependentResources();

    if (!_deviceResources->setWindowSize(width, height)) {
      LOG(WARNING) << "Device resources failed to handle WindowSizeChanged event.";
    }

    createWindowSizeDependentResources();
  }

  // Properties
  void createDeviceDependentResources() {
    try {
      _deviceResources->createDeviceDependentResources();
      _scene->createDeviceDependentResources();
    } catch (std::exception& e) {
      LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
      _fatalError = true;
    }
  }

  void createWindowSizeDependentResources() {
    try {
      _deviceResources->recreateWindowSizeDependentResources();
      int width, height;
      if (!_deviceResources->getWindowSize(width, height)) {
        LOG(WARNING) << "createWindowSizeDependentResources: Failed to get last window size.";
        return;
      }

      _scene->createWindowSizeDependentResources(
          _hwnd, std::max<UINT>(width, 1), std::max<UINT>(height, 1));
    } catch (std::exception& e) {
      LOG(FATAL) << "Failed to create window size dependent resources: " << e.what();
      _fatalError = true;
    }
  }

  void destroyDeviceDependentResources() {
    _scene->destroyDeviceDependentResources();
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

  static void getDefaultWindowSize(int w, int h) {

    // Get dimensions of the primary monitor
    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);

    // Set window to 75% of the desktop size
    w = std::max(320l, (rc.right * 3) / 4);
    h = std::max(200l, (rc.bottom * 3) / 4);
  }

 protected:
  App* const _app;
  IDevice_resources* const _deviceResources;
  Scene_device_resource_aware* const _scene;
  HWND const _hwnd;
  bool _fatalError;
  StepTimer _timer;
};
} // namespace oe

// Do NOT do this in 64 bit builds, it is incompatible with json.hpp for some reason.
// EXTERN_C IMAGE_DOS_HEADER __ImageBase;
//#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Entry point
int oe::App::run(const oe::App_start_settings& settings) {
  // Initialize logger
  std::string logFileName;
  auto logWorker = g3::LogWorker::createLogWorker();
  {
    auto execName = oe::utf8_encode(__wargv[0]);
    auto pos = execName.find_last_of("\\");
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

  // Create the scene
  auto scene = Scene_device_resource_aware(*deviceResources);

  int progReturnVal = 1;
  auto appDeviceContext = App_impl(this, hwnd, deviceResources.get(), &scene);
  _impl = &appDeviceContext;

  bool sceneConfigured = false;
  do {
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    try {

      scene.configure();
      sceneConfigured = true;

      scene.manager<oe::IDev_tools_manager>().setVectorLog(vectorLog.get());
      onSceneConfigured(scene);
    } catch (const std::exception& e) {
      LOG(WARNING) << "Failed to configure scene: " << e.what();
      progReturnVal = 1;
      break;
    }

    try {
      // Some things need setting before initialization.
      scene.manager<IUser_interface_manager>().preInit_setUIScale(
          static_cast<float>(getScreenDpi()) / 96.0f);

      scene.initialize();

      onSceneInitialized(scene);
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
          onScenePreTick(scene);
          appDeviceContext.onTick();

          onScenePreRender(scene);
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
      scene.destroyWindowSizeDependentResources();
      scene.destroyDeviceDependentResources();

      scene.manager<oe::IDev_tools_manager>().setVectorLog(nullptr);

      onSceneShutdown(scene);
      scene.shutdown();
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

void oe::App::getDefaultWindowSize(int& w, int& h) const { _impl->getDefaultWindowSize(w, h); }

// Get DPI of the screen that the current window is on
int oe::App::getScreenDpi() const {
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

  auto* app = reinterpret_cast<oe::App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  oe::App_impl* appImpl = nullptr;
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
void ExitGame() { PostQuitMessage(0); }