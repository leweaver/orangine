//
// Game.h
//

#pragma once

#include <OeApp/App.h>
#include <OeCore/DeviceResources.h>
#include <OeCore/Scene.h>
#include <OeCore/StepTimer.h>

#include <memory>

namespace oe {
class Entity;
}

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public oe::App, public DX::IDeviceNotify {
 public:
  Game();
  ~Game();

  // Initialization and management
  void Configure(HWND window, int dpi, int width, int height);
  void Initialize();
  void InitRasterizer();

  // Signal that the game can no longer run and should exit.
  bool hasFatalError() const { return m_fatalError; }

  // Basic game loop
  void Tick();
  void Render();

  // IDeviceNotify
  void OnDeviceLost() override;
  void OnDeviceRestored() override;

  // Messages
  inline bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) const;
  void OnActivated();
  void OnDeactivated();
  void OnSuspending();
  void OnResuming();
  void OnWindowMoved();
  void OnWindowSizeChanged(int width, int height);

  // Properties
  static void GetDefaultSize(int& width, int& height);
  oe::Scene& scene() const { return *m_scene; }

 private:

  void createDeviceDependentResources();
  void createWindowSizeDependentResources();

  // Device resources.
  std::unique_ptr<DX::DeviceResources> m_deviceResources;

  // Rendering loop timer.
  DX::StepTimer m_timer;

  // Services
  std::unique_ptr<oe::Scene_device_resource_aware> m_scene;

  bool m_fatalError;
  HWND _window;
};

bool Game::processMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
  if (m_scene != nullptr) {
    return m_scene->processMessage(message, wParam, lParam);
  }
  return false;
}