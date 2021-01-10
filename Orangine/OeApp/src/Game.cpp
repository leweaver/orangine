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

Game::Game() : m_fatalError(false) {
  m_deviceResources =
      std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R24G8_TYPELESS);
  m_deviceResources->RegisterDeviceNotify(this);
}

#pragma region Direct3D Resources

// Allocate all memory resources that change on a window SizeChanged event.

void Game::OnDeviceLost() {
  m_scene->destroyWindowSizeDependentResources();
  m_scene->destroyDeviceDependentResources();
}

void Game::OnDeviceRestored() {
  createDeviceDependentResources();
  createWindowSizeDependentResources();
}
#pragma endregion
