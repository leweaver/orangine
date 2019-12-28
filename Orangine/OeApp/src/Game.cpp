//
// Game.cpp
//

#include "pch.h"

#include "Game.h"

#include "OeAppConfig.h"
#include <g3log/g3log.hpp>

#include <filesystem>

extern void ExitGame();

using namespace DirectX;
using namespace oe;

using Microsoft::WRL::ComPtr;

Game::Game() : m_fatalError(false)
{
  m_deviceResources =
      std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R24G8_TYPELESS);
  m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
  if (m_scene) {
    m_scene->destroyWindowSizeDependentResources();
    m_scene->destroyDeviceDependentResources();
    m_scene->shutdown();
  }

  ComPtr<ID3D11Device> device = m_deviceResources->GetD3DDevice();
  ComPtr<ID3D11Debug> d3DDebug;
  if (device && SUCCEEDED(device.As(&d3DDebug))) {
    // d3DDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
  }

  m_deviceResources.reset();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int dpi, int width, int height)
{
  _window = window;
  m_deviceResources->SetWindow(window, width, height);

  m_scene = std::make_unique<Scene_device_resource_aware>(*(m_deviceResources.get()));

  // Some things need setting before initialization.
  m_scene->manager<IUser_interface_manager>().setUIScale(static_cast<float>(dpi) / 96.0f);

  try {
    m_scene->initialize();
  }
  catch (std::exception& e) {
    LOG(WARNING) << "Failed to init scene: " << e.what();
    m_fatalError = true;
    return;
  }

  try {
    createDeviceDependentResources();
    createWindowSizeDependentResources();
  }
  catch (const std::exception& e) {
    LOG(WARNING) << "Failed to create device resources: " << e.what();
    m_fatalError = true;
    return;
  }
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{

  // Updates the world.
  m_timer.Tick([&]() {
    m_scene->tick(m_timer);
  });
}

#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
  // Don't try to render anything before the first Update.
  if (m_timer.GetFrameCount() == 0) {
    return;
  }

  m_scene->manager<IRender_step_manager>().render(m_scene->mainCamera());
  m_scene->manager<IUser_interface_manager>().render();

  // Show the new frame.
  m_deviceResources->Present();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
  // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
  // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
  // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
  m_timer.ResetElapsedTime();

  // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
  const auto r = m_deviceResources->GetOutputSize();
  LOG(INFO) << "Window moved. new size: " << r.right << ", " << r.bottom;

  m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
  const auto oldSize = m_deviceResources->GetOutputSize();
  if (oldSize.right == width && oldSize.bottom == height)
    return;

  LOG(INFO) << "Window size changed: " << width << ", " << height;

  m_scene->destroyWindowSizeDependentResources();

  if (!m_deviceResources->WindowSizeChanged(width, height)) {
    LOG(WARNING) << "Device resources failed to handle WindowSizeChanged event.";
  }

  createWindowSizeDependentResources();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
  // Get dimensions of the primary monitor
  RECT rc;
  GetWindowRect(GetDesktopWindow(), &rc);

  // Set window to 75% of the desktop size
  width = std::max(320l, (rc.right * 3) / 4);
  height = std::max(200l, (rc.bottom * 3) / 4);
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::createDeviceDependentResources()
{
  try {
    m_deviceResources->CreateDeviceResources();
    m_scene->createDeviceDependentResources();
  }
  catch (std::exception& e) {
    LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
    m_fatalError = true;
  }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::createWindowSizeDependentResources()
{
  try {
    m_deviceResources->CreateWindowSizeDependentResources();
    const auto outputSize = m_deviceResources->GetOutputSize();
    m_scene->createWindowSizeDependentResources(
        _window, std::max<UINT>(outputSize.right - outputSize.left, 1),
        std::max<UINT>(outputSize.bottom - outputSize.top, 1));
  }
  catch (std::exception& e) {
    LOG(FATAL) << "Failed to create window size dependent resources: " << e.what();
    m_fatalError = true;
  }
}

void Game::OnDeviceLost()
{
  m_scene->destroyWindowSizeDependentResources();
  m_scene->destroyDeviceDependentResources();
}

void Game::OnDeviceRestored()
{
  createDeviceDependentResources();

  createWindowSizeDependentResources();
}
#pragma endregion
