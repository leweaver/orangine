//
// Game.h
//

#pragma once

#include "../Engine/DeviceResources.h"
#include "../Engine/StepTimer.h"
#include "../Engine/Scene.h"

#include <memory>

namespace oe
{
	class Entity;
}

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
public:

    Game();
	~Game();

	void CreateSceneLeverArm();
	void CreateGeometricPrimitives();
	void CreateSceneCubeSatellite();
	std::shared_ptr<oe::Entity> LoadGLTF(std::string gltfName, bool animate);
	void CreateSceneMetalRoughSpheres(bool animate);
	void CreateLights();
	void CreateCamera(bool animate);

	// Initialization and management
    void Initialize(HWND window, int width, int height);
	void InitRasterizer();

	// Signal that the game can no longer run and should exit.
	bool hasFatalError() const { return m_fatalError; };

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
	inline void processMessage(UINT message, WPARAM wParam, LPARAM lParam) const;
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void createDeviceDependentResources();
    void createWindowSizeDependentResources();

	void AddCubeToEntity(oe::Entity& entity, DirectX::SimpleMath::Vector3 animationSpeed, DirectX::SimpleMath::Vector3 localScale, DirectX::SimpleMath::Vector3 localPosition) const;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	// Services
	std::unique_ptr<oe::Scene_device_resource_aware> m_scene;
	
	bool									m_fatalError;
	HWND									_window;
};

void Game::processMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
	if (m_scene != nullptr) {
		m_scene->processMessage(message, wParam, lParam);
	}
}