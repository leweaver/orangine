//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "SceneGraphManager.h"
#include "Scene.h"
#include "TestComponent.h"
#include "EntityRenderManager.h"

extern void ExitGame();

using namespace DirectX;
using namespace OE;

using Microsoft::WRL::ComPtr;

Game::Game()
	: m_fatalError(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
	if (m_scene) {
		m_scene->GetEntityRenderManger().destroyDeviceDependentResources();
		m_scene->Shutdown();
	}

	// Uncomment to output debug information on shutdown
	/*
	Microsoft::WRL::ComPtr<ID3D11Debug> d3D11Debug;	
	HRESULT hr = m_deviceResources->GetD3DDevice()->QueryInterface(IID_PPV_ARGS(&d3D11Debug));
	if (d3D11Debug != nullptr)
	{
		d3D11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
	*/
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_scene = std::make_unique<Scene>(*(m_deviceResources.get()));

    m_deviceResources->SetWindow(window, width, height);

	{
		// TODO: Change the timer settings if you want something other than the default variable timestep mode.
		// e.g. for 60 FPS fixed timestep update logic, call:
		/*
		m_timer.SetFixedTimeStep(true);
		m_timer.SetTargetElapsedSeconds(1.0 / 60);
		*/

		SceneGraphManager& entityManager = m_scene->GetSceneGraphManager();
		Entity& root1 = entityManager.Instantiate("Root 1");
		Entity& child1 = entityManager.Instantiate("Child 1", root1);
		Entity& child2 = entityManager.Instantiate("Child 2", root1);
		Entity& child3 = entityManager.Instantiate("Child 3", root1);

		root1.AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.1250f, 0.06f, 0.0f));

		AddCubeToEntity(child1, XMVectorSet(0.5f, 0.0f, 0.0f, 0.0f),  XMVectorSet(2, 1, 1, 1), XMVectorSet(4, 0, 0, 1));
		AddCubeToEntity(child2, XMVectorSet(0.0f, 0.5f, 0.0f, 0.0f), XMVectorSet(1, 2, 1, 1), XMVectorSet(0, 0, 0, 1));
		AddCubeToEntity(child3, XMVectorSet(0.0f, 0.0f, 0.5f, 0.0f), XMVectorSet(1, 1, 2, 1), XMVectorSet(-4, 0, 0, 1));
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();
}

void Game::AddCubeToEntity(Entity& entity, FXMVECTOR animationSpeed, FXMVECTOR localScale, FXMVECTOR localPosition) const
{
	entity.AddComponent<TestComponent>().SetSpeed(animationSpeed);

	entity.SetLocalScale(localScale);
	entity.SetLocalPosition(localPosition);
	
	m_scene->LoadEntities("data/meshes/Cube/Cube.gltf", entity);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    // TODO: Add your game logic here.
	m_scene->Tick(timer);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }
	
    // TODO: Add your rendering code here.
	m_scene->GetEntityRenderManger().render();
	
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
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	try {
		m_scene->GetEntityRenderManger().createDeviceDependentResources();
	}
	catch (std::runtime_error &e)
	{
		LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
		m_fatalError = true;
	}
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	try
	{
		m_scene->GetEntityRenderManger().createWindowSizeDependentResources();
	}
	catch (std::runtime_error &e)
	{
		LOG(FATAL) << "Failed to create window size dependent resources: " << e.what();
		m_fatalError = true;
	}
}

void Game::OnDeviceLost()
{
	m_scene->GetEntityRenderManger().destroyDeviceDependentResources();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
