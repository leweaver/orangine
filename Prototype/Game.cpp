//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "SceneGraphManager.h"
#include "Scene.h"
#include "TestComponent.h"
#include "EntityRenderManager.h"
#include "CameraComponent.h"
#include "LightComponent.h"

extern void ExitGame();

using namespace DirectX;
using namespace SimpleMath;
using namespace OE;

using Microsoft::WRL::ComPtr;

Game::Game()
	: m_fatalError(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R24G8_TYPELESS);
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

void Game::CreateSceneCubeSatellite()
{
	SceneGraphManager& entityManager = m_scene->GetSceneGraphManager();
	const auto &root1 = entityManager.Instantiate("Root 1");
	//root1->AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.1250f, 0.06f, 0.0f));

	const auto &child1 = entityManager.Instantiate("Child 1", *root1);
	const auto &child2 = entityManager.Instantiate("Child 2", *root1);
	const auto &child3 = entityManager.Instantiate("Child 3", *root1);

	AddCubeToEntity(*child1, {0.5f, 0.0f, 0.0f}, {2, 1, 1}, { 4, 0, 0});
	AddCubeToEntity(*child2, {0.0f, 0.5f, 0.0f}, {1, 2, 1}, { 0, 0, 0});
	AddCubeToEntity(*child3, {0.0f, 0.0f, 0.5f}, {1, 1, 2}, {-4, 0, 0});
}

void Game::CreateCamera()
{
	SceneGraphManager& entityManager = m_scene->GetSceneGraphManager();

	auto cameraDollyAnchor = entityManager.Instantiate("CameraDollyAnchor");
	cameraDollyAnchor->AddComponent<TestComponent>().SetSpeed({0.0f, 0.1f, 0.0f});
	
	auto camera = entityManager.Instantiate("Camera", *cameraDollyAnchor);
	camera->SetPosition(Vector3(0.0f, 0.0f, 3.0f));
	
	cameraDollyAnchor->Update();

	auto &component = camera->AddComponent<CameraComponent>();
	component.SetFov(XMConvertToRadians(60.0f));
	component.SetFarPlane(30.0f);
	component.SetNearPlane(0.1f);

	camera->LookAt({ 5, 0, 5 }, Vector3::Up);

	m_scene->SetMainCamera(camera);
}

void Game::CreateLights()
{
	SceneGraphManager& entityManager = m_scene->GetSceneGraphManager();
	int lightCount = 0;
	auto createDirLight = [&entityManager, &lightCount](const Vector3 &normal, const Color &color, float intensity)
	{
		auto lightEntity = entityManager.Instantiate("Directional Light " + to_string(++lightCount));
		auto &component = lightEntity->AddComponent<DirectionalLightComponent>();
		component.setColor(color);
		component.setIntensity(intensity);
		
		if (normal != Vector3::Forward)
		{
			Vector3 axis;
			if (normal == Vector3::Backward)
				axis = Vector3::Up;
			else
			{
				axis = Vector3::Forward.Cross(normal);
				axis.Normalize();
			}

			assert(normal.LengthSquared() != 0);
			float angle = acos(Vector3::Forward.Dot(normal) / normal.Length());
			lightEntity->SetRotation(Quaternion::CreateFromAxisAngle(axis, angle));
		}
	};
	auto createPointLight = [&entityManager, &lightCount](const Vector3 &position, const Color &color, float intensity)
	{
		auto lightEntity = entityManager.Instantiate("Point Light " + to_string(++lightCount));
		auto &component = lightEntity->AddComponent<PointLightComponent>();
		component.setColor(color);
		component.setIntensity(intensity);
		lightEntity->SetPosition(position);
	};

	lightCount = 0;
	createDirLight({0, 0, -1}, {0, 1, 1}, 1);
	createDirLight({0, 0, 1}, {0, 0, 1}, 1);

	createDirLight({1, 0, 0}, {0, 1, 1}, 1);
	createDirLight({-1, 0, 0}, {1, 0, 0}, 1);

	createDirLight({0, 1, 0}, {0, 1, 1}, 1);
	createDirLight({0, -1, 0}, {0, 1, 0}, 1);

	lightCount = 0;
	createPointLight({ 3, 0, 3 }, { 1, 1, 1 }, 1);
}

void Game::CreateSceneLeverArm()
{
	SceneGraphManager& entityManager = m_scene->GetSceneGraphManager();
	const auto &root1 = entityManager.Instantiate("Root 1");
	root1->SetPosition({ 5, 0, 5 });

	const auto &child1 = entityManager.Instantiate("Child 1", *root1);
	const auto &child2 = entityManager.Instantiate("Child 2", *child1);
	const auto &child3 = entityManager.Instantiate("Child 3", *child2);

	AddCubeToEntity(*child1, { 0, 0, 0.1f }, { 0.5f, 0.5f, 1.0f }, { 0, 0, 0 });
	AddCubeToEntity(*child2, { 0, 0.25, 0 }, { 1.0f, 2.0f, 0.5f }, { 2, 0, 0 });
	AddCubeToEntity(*child3, { 0.5f, 0, 0 }, { 2.0f, 0.50f, 1.0f }, { 0, 2, 0});
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


		//CreateSceneCubeSatellite();


		CreateSceneLeverArm();

		CreateCamera();
		CreateLights();
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();
}

void Game::AddCubeToEntity(Entity& entity, Vector3 animationSpeed, Vector3 localScale, Vector3 localPosition) const
{
	entity.AddComponent<TestComponent>().SetSpeed(animationSpeed);

	entity.SetScale(localScale);
	entity.SetPosition(localPosition);
	
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
	catch (std::exception &e)
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
	catch (std::exception &e)
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
