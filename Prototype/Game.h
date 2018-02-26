//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include <memory>

#include "Scene.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
public:

    Game();
	~Game();

	void CreateSceneLeverArm();
	void CreateSceneCubeSatellite();
	void CreateLights();
	void CreateCamera();

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

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

	void AddCubeToEntity(OE::Entity& entity, DirectX::SimpleMath::Vector3 animationSpeed, DirectX::SimpleMath::Vector3 localScale, DirectX::SimpleMath::Vector3 localPosition) const;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	// Services
	std::unique_ptr<OE::Scene>              m_scene;
	
	bool									m_fatalError;
};