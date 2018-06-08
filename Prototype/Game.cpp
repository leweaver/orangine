//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "Scene_graph_manager.h"
#include "Scene.h"
#include "Test_component.h"
#include "Entity_render_manager.h"
#include "Camera_component.h"
#include "Light_component.h"

extern void ExitGame();

using namespace DirectX;
using namespace SimpleMath;
using namespace oe;

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
		m_scene->entityRenderManger().destroyDeviceDependentResources();
		m_scene->shutdown();
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
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();
	const auto &root1 = entityManager.instantiate("Root 1");
	//root1->AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.1250f, 0.06f, 0.0f));

	const auto &child1 = entityManager.instantiate("Child 1", *root1);
	const auto &child2 = entityManager.instantiate("Child 2", *root1);
	const auto &child3 = entityManager.instantiate("Child 3", *root1);

	AddCubeToEntity(*child1, {0.5f, 0.0f, 0.0f}, {2, 1, 1}, { 4, 0, 0});
	AddCubeToEntity(*child2, {0.0f, 0.5f, 0.0f}, {1, 2, 1}, { 0, 0, 0});
	AddCubeToEntity(*child3, {0.0f, 0.0f, 0.5f}, {1, 1, 2}, {-4, 0, 0});
}

std::shared_ptr<Entity> Game::LoadGLTF(std::string gltfName, bool animate)
{
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();
	auto root = entityManager.instantiate("Root");

	if (animate)
		root->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });

	//m_scene->loadEntities("data/meshes/" + gltfName + "/" + gltfName + ".gltf", *root);
	m_scene->loadEntities("../../glTF-Sample-Models/2.0/" + gltfName + "/glTF/" + gltfName + ".gltf", *root);

	return root;
}

void Game::CreateSceneMetalRoughSpheres(bool animate)
{
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();
	const auto &root1 = entityManager.instantiate("Root");
	//root1->SetPosition({ 0, 0, -10 });
	root1->setScale({ 1, 1, 1 });

	if (animate)
		root1->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });

	m_scene->loadEntities("data/meshes/MetalRoughSpheres/MetalRoughSpheres.gltf", *root1);
}

void Game::CreateCamera(bool animate)
{
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();

	auto cameraDollyAnchor = entityManager.instantiate("CameraDollyAnchor");
	if (animate)
		cameraDollyAnchor->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});
	
	auto camera = entityManager.instantiate("Camera", *cameraDollyAnchor);
	camera->setPosition(Vector3(0.0f, 0.0f, 15));
	
	cameraDollyAnchor->update();

	auto &component = camera->addComponent<Camera_component>();
	component.setFov(XMConvertToRadians(60.0f));
	component.setFarPlane(100.0f);
	component.setNearPlane(0.1f);

	camera->lookAt({ 0, 0, 0 }, Vector3::Up);

	m_scene->setMainCamera(camera);
}

void Game::CreateLights()
{
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();
	int lightCount = 0;
	auto createDirLight = [&entityManager, &lightCount](const Vector3 &normal, const Color &color, float intensity)
	{
		auto lightEntity = entityManager.instantiate("Directional Light " + std::to_string(++lightCount));
		auto &component = lightEntity->addComponent<Directional_light_component>();
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
			lightEntity->setRotation(Quaternion::CreateFromAxisAngle(axis, angle));
		}
		return lightEntity;
	};
	auto createPointLight = [&entityManager, &lightCount](const Vector3 &position, const Color &color, float intensity)
	{
		auto lightEntity = entityManager.instantiate("Point Light " + std::to_string(++lightCount));
		auto &component = lightEntity->addComponent<Point_light_component>();
		component.setColor(color);
		component.setIntensity(intensity);
		lightEntity->setPosition(position);
		return lightEntity;
	};
	auto createAmbientLight = [&entityManager, &lightCount](const Color &color, float intensity)
	{
		auto lightEntity = entityManager.instantiate("Ambient Light " + std::to_string(++lightCount));
		auto &component = lightEntity->addComponent<Ambient_light_component>();
		component.setColor(color);
		component.setIntensity(intensity);
		return lightEntity;
	};

	const auto &lightRoot = entityManager.instantiate("Light Root");
	lightRoot->setPosition({ 0, 0, 0 });
	//createDirLight({ 0.5, 0, -1 }, { 1, 1, 1 }, 0.35)->SetParent(*lightRoot);

	createAmbientLight({ 1, 1, 1 }, 1)->setParent(*lightRoot);

	if (true)
	{
		createDirLight({ -0.707, 0, -0.707 }, { 1, 1, 1 }, 1)->setParent(*lightRoot);
		createDirLight({ -0.666, -0.333, 0.666 }, { 1, 0, 1 }, 0.75)->setParent(*lightRoot);
	}
	else
	{
		createPointLight({ 10, 0, 10 }, { 1, 1, 1 }, 13)->setParent(*lightRoot);
		createPointLight({ 10, 5, -10 }, { 1, 0, 1 }, 20)->setParent(*lightRoot);
	}
}

void Game::CreateSceneLeverArm()
{
	Scene_graph_manager& entityManager = m_scene->sceneGraphManager();
	const auto &root1 = entityManager.instantiate("Root 1");
	root1->setPosition({ 5, 0, 5 });

	const auto &child1 = entityManager.instantiate("Child 1", *root1);
	const auto &child2 = entityManager.instantiate("Child 2", *child1);
	const auto &child3 = entityManager.instantiate("Child 3", *child2);

	AddCubeToEntity(*child1, { 0, 0, 0.1f }, { 0.5f, 0.5f, 1.0f }, { 0, 0, 0 });
	AddCubeToEntity(*child2, { 0, 0.25, 0 }, { 1.0f, 2.0f, 0.5f }, { 2, 0, 0 });
	AddCubeToEntity(*child3, { 0.5f, 0, 0 }, { 2.0f, 0.50f, 1.0f }, { 0, 2, 0});
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_scene = std::make_unique<Scene>(*(m_deviceResources.get()));

    m_deviceResources->SetWindow(window, width, height);

	try
	{
		// TODO: Change the timer settings if you want something other than the default variable timestep mode.
		// e.g. for 60 FPS fixed timestep update logic, call:
		/*
		m_timer.SetFixedTimeStep(true);
		m_timer.SetTargetElapsedSeconds(1.0 / 60);
		*/


		//CreateSceneCubeSatellite();
		//CreateSceneLeverArm();
		//LoadGLTF("Avocado", true)->setScale({ 120, 120, 120 });
		
		//LoadGLTF("NormalTangentTest", false)->setScale({ 7, 7, 7 });
		//LoadGLTF("AlphaBlendModeTest", false)->setScale({3, 3, 3});
		LoadGLTF("FlightHelmet", true)->setScale({ 10, 10, 10 });
		//LoadGLTF("WaterBottle", true)->setScale({ 80, 80, 80 });
		
		//CreateSceneMetalRoughSpheres(true);

		CreateCamera(false);
		CreateLights();
	}
	catch (const std::exception &e)
	{
		LOG(FATAL) << "Failed to load scene: " << e.what();
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();
}

void Game::AddCubeToEntity(Entity& entity, Vector3 animationSpeed, Vector3 localScale, Vector3 localPosition) const
{
	entity.addComponent<Test_component>().setSpeed(animationSpeed);

	entity.setScale(localScale);
	entity.setPosition(localPosition);
	
	m_scene->loadEntities("data/meshes/Cube/Cube.gltf", entity);
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
	m_scene->tick(timer);
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
	m_scene->entityRenderManger().render();
	
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

	m_scene->entityRenderManger().destroyWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1024 * 2;
    height = 800 * 2;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	try {
		m_scene->entityRenderManger().createDeviceDependentResources();
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
		m_scene->entityRenderManger().createWindowSizeDependentResources();
	}
	catch (std::exception &e)
	{
		LOG(FATAL) << "Failed to create window size dependent resources: " << e.what();
		m_fatalError = true;
	}
}

void Game::OnDeviceLost()
{
	m_scene->entityRenderManger().destroyWindowSizeDependentResources();
	m_scene->entityRenderManger().destroyDeviceDependentResources();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
