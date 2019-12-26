//
// Main.cpp
//

#include "pch.h"

#include "Game.h"
#include "LogFileSink.h"
#include "VectorLogSink.h"
#include <OeApp/App.h>

#include <OeCore/EngineUtils.h>

#include <g3log/logworker.hpp>

#include <filesystem>
#include <wrl/wrappers/corewrappers.h>

using namespace DirectX;

namespace {
std::unique_ptr<Game> g_game;
};

const std::string path_to_log_file = "./";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
typedef void(__cdecl* GamePluginInitFn)(oe::Scene&);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
IMAGE_DOS_HEADER __ImageBase;
}

#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Entry point
int oe::App::run(const oe::App_start_settings& settings)
{
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
  logWorker->addSink(move(fileLogSink), &oe::LogFileSink::fileWrite);
  auto vectorLog = std::make_shared<oe::VectorLog>(100);
  auto vectorLogSink = std::make_unique<oe::VectorLogSink>(vectorLog);
  logWorker->addSink(move(vectorLogSink), &oe::VectorLogSink::append);

  g3::initializeLogging(logWorker.get());

  if (!XMVerifyCPUSupport()) {
    LOG(WARNING) << "Unsupported CPU";
    return 1;
  }

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

  HINSTANCE hInstance = HINST_THISCOMPONENT;
  g_game = std::make_unique<Game>();

  // Register class and create window
  {
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
    wcex.lpszClassName = L"ViewerAppWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, L"IDI_ICON");
    if (!RegisterClassEx(&wcex)) {
      LOG(WARNING) << "Failed to register window class: " << getlasterror_to_str();
      return 1;
    }

    // Create window
    int w, h;
    g_game->GetDefaultSize(w, h);

    RECT rc;
    rc.top = 0;
    rc.left = 0;
    rc.right = static_cast<LONG>(w);
    rc.bottom = static_cast<LONG>(h);

    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    auto dwExStyle = settings.fullScreen ? WS_EX_TOPMOST : 0;
    auto dwStyle = settings.fullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    HWND hwnd = CreateWindowEx(dwExStyle, L"ViewerAppWindowClass", L"ViewerApp", dwStyle,
                               CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                               nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
      LOG(WARNING) << "Failed to create window: " << getlasterror_to_str();
      return 1;
    }

    ShowWindow(hwnd, settings.fullScreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

    GetClientRect(hwnd, &rc);
    g_game->Initialize(hwnd, GetDpiForWindow(hwnd), rc.right - rc.left, rc.bottom - rc.top);

    if (!g_game->hasFatalError()) {
      g_game->scene().manager<oe::IDev_tools_manager>().setVectorLog(vectorLog.get());
    }

    try {
      onSceneInitialized(g_game->scene());
    }
    catch (const std::exception& e) {
      LOG(WARNING) << "Failed to load scene: " << e.what();
      return 1;
    }
  }

  // Main message loop
  MSG msg = {};
  while (WM_QUIT != msg.message && !g_game->hasFatalError()) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      onSceneTick(g_game->scene());
      g_game->Tick();
    }
  }

  g_game->scene().manager<oe::IDev_tools_manager>().setVectorLog(nullptr);
  g_game.reset();

  CoUninitialize();

  return (int)msg.wParam;
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;

  static bool s_in_sizemove = false;
  static bool s_in_suspend = false;
  static bool s_minimized = false;
  static bool s_fullscreen = false;
  // TODO: Set s_fullscreen to true if defaulting to fullscreen.

  auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

  // Give the game managers a change to process messages (for mouse input etc)
  if (game != nullptr) {
    if (game->processMessage(message, wParam, lParam)) {
      return true;
    }
  }

  switch (message) {
  case WM_PAINT:
    if (s_in_sizemove && game) {
      game->Tick();
    }
    else {
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
    }
    break;

  case WM_MOVE:
    if (game) {
      game->OnWindowMoved();
    }
    break;

  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) {
      if (!s_minimized) {
        s_minimized = true;
        if (!s_in_suspend && game)
          game->OnSuspending();
        s_in_suspend = true;
      }
    }
    else if (s_minimized) {
      s_minimized = false;
      if (s_in_suspend && game)
        game->OnResuming();
      s_in_suspend = false;
    }
    else if (!s_in_sizemove && game) {
      game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
    }
    break;

  case WM_ENTERSIZEMOVE:
    s_in_sizemove = true;
    break;

  case WM_EXITSIZEMOVE:
    s_in_sizemove = false;
    if (game) {
      RECT rc;
      GetClientRect(hWnd, &rc);

      game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
    }
    break;

  case WM_GETMINMAXINFO: {
    auto info = reinterpret_cast<MINMAXINFO*>(lParam);
    info->ptMinTrackSize.x = 320;
    info->ptMinTrackSize.y = 200;
  } break;

  case WM_ACTIVATEAPP:
    if (game) {
      if (wParam) {
        game->OnActivated();
      }
      else {
        game->OnDeactivated();
      }
    }
    break;

  case WM_POWERBROADCAST:
    switch (wParam) {
    case PBT_APMQUERYSUSPEND:
      if (!s_in_suspend && game)
        game->OnSuspending();
      s_in_suspend = true;
      return TRUE;

    case PBT_APMRESUMESUSPEND:
      if (!s_minimized) {
        if (s_in_suspend && game)
          game->OnResuming();
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

        int width = 800;
        int height = 600;
        if (game)
          game->GetDefaultSize(width, height);

        ShowWindow(hWnd, SW_SHOWNORMAL);

        SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
      }
      else {
        SetWindowLongPtr(hWnd, GWL_STYLE, 0);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

        SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

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