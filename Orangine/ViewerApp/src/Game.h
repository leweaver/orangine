//
// Game.h
//

#pragma once

#include "OeCore/DeviceResources.h"
#include "OeCore/StepTimer.h"
#include "OeCore/Scene.h"

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

    explicit Game();
	~Game();

	void CreateSceneLeverArm();
	void CreateScripts();
	void CreateShadowTestScene();
	void CreateSceneCubeSatellite();
	std::shared_ptr<oe::Entity> LoadGLTF(std::string gltfName, bool animate);
	std::shared_ptr<oe::Entity> LoadGLTFToEntity(std::string gltfName, std::shared_ptr<oe::Entity> root);
	void CreateSceneMetalRoughSpheres(bool animate);
	void CreateLights();
	void CreateCamera(bool animate);

	// Initialization and management
    void Initialize(HWND window, int dpi, int width, int height);
	void InitRasterizer();

	// Signal that the game can no longer run and should exit.
	bool hasFatalError() const { return m_fatalError; }

    // Basic game loop
    void Tick();

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
    void GetDefaultSize( int& width, int& height ) const;
    oe::Scene& scene() const { return *m_scene; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void createDeviceDependentResources();
    void createWindowSizeDependentResources();

	void AddCubeToEntity(std::shared_ptr<oe::Entity> entity, SSE::Vector3 animationSpeed, DirectX::SimpleMath::Vector3 localScale, DirectX::SimpleMath::Vector3 localPosition);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	// Services
	std::unique_ptr<oe::Scene_device_resource_aware> m_scene;
	
	bool									m_fatalError;
	HWND									_window;
};

bool Game::processMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
	if (m_scene != nullptr) {
		return m_scene->processMessage(message, wParam, lParam);
	}
    return false;
}