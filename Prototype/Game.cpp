//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

#include "../Engine/Scene_graph_manager.h"
#include "../Engine/Scene.h"
#include "../Engine/Test_component.h"
#include "../Engine/Entity_render_manager.h"
#include "../Engine/Camera_component.h"
#include "../Engine/Light_component.h"
#include "../Engine/Renderable_component.h"
#include "../Engine/PBR_material.h"
#include "../Engine/Unlit_material.h"
#include "../Engine/Collision.h"

extern void ExitGame();

using namespace DirectX;
using namespace SimpleMath;
using namespace oe;

using Microsoft::WRL::ComPtr;

std::shared_ptr<Entity> g_debugEntity;

Game::Game()
	: m_fatalError(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R24G8_TYPELESS);
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
	g_debugEntity.reset();
	if (m_scene) {
		m_scene->manager<IEntity_render_manager>().destroyDeviceDependentResources();
		m_scene->shutdown();
	}

	/*
	ComPtr<ID3D11Device> device = m_deviceResources->GetD3DDevice();
	m_deviceResources.reset();

	ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(device.As(&d3dDebug))) {
		HRESULT hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
	*/
}

void Game::CreateSceneCubeSatellite()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
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
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	auto root = entityManager.instantiate("Root");

	if (animate)
		root->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });

	//m_scene->loadEntities("data/meshes/" + gltfName + "/" + gltfName + ".gltf", *root);
	m_scene->loadEntities("../../glTF-Sample-Models/2.0/" + gltfName + "/glTF/" + gltfName + ".gltf", *root);

	return root;
}

void Game::CreateSceneMetalRoughSpheres(bool animate)
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto &root1 = entityManager.instantiate("Root");
	//root1->SetPosition({ 0, 0, -10 });
	root1->setScale({ 1, 1, 1 });

	if (animate)
		root1->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });

	m_scene->loadEntities("data/meshes/MetalRoughSpheres/MetalRoughSpheres.gltf", *root1);
}

void Game::CreateCamera(bool animate)
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();

	auto cameraDollyAnchor = entityManager.instantiate("CameraDollyAnchor");
	if (animate)
		cameraDollyAnchor->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});
	
	auto camera = entityManager.instantiate("Camera", *cameraDollyAnchor);
	camera->setPosition(Vector3(0.0f, 0.0f, 15));
	
	cameraDollyAnchor->computeWorldTransform();

	auto &component = camera->addComponent<Camera_component>();
	component.setFov(XMConvertToRadians(60.0f));
	component.setFarPlane(100.0f);
	component.setNearPlane(0.1f);

	camera->lookAt({ 0, 0, 0 }, Vector3::Up);

	m_scene->setMainCamera(camera);


}

void Game::CreateLights()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
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

	createAmbientLight({ 1, 1, 1 }, 0.2f)->setParent(*lightRoot);

	if (true)
	{
		createDirLight({ -0.707, 0, -0.707 }, { 1, 1, 1 }, 2)->setParent(*lightRoot);
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
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto &root1 = entityManager.instantiate("Root 1");
	root1->setPosition({ 5, 0, 5 });

	const auto &child1 = entityManager.instantiate("Child 1", *root1);
	const auto &child2 = entityManager.instantiate("Child 2", *child1);
	const auto &child3 = entityManager.instantiate("Child 3", *child2);

	AddCubeToEntity(*child1, { 0, 0, 0.1f }, { 0.5f, 0.5f, 1.0f }, { 0, 0, 0 });
	AddCubeToEntity(*child2, { 0, 0.25, 0 }, { 1.0f, 2.0f, 0.5f }, { 2, 0, 0 });
	AddCubeToEntity(*child3, { 0.5f, 0, 0 }, { 2.0f, 0.50f, 1.0f }, { 0, 2, 0});
}

void Game::CreateGeometricPrimitives()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto &root1 = entityManager.instantiate("Primitives");


	Primitive_mesh_data_factory pmdf;
	{
		const auto &child1 = entityManager.instantiate("Primitive Child 1", *root1);
		std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
		material->setBaseColor(Color(Colors::GreenYellow));
		material->setMetallicFactor(1.0f);
		material->setRoughnessFactor(0.0f);
		
		auto& renderable = child1->addComponent<Renderable_component>();
		renderable.setMaterial(std::unique_ptr<Material>(material.release()));
		renderable.setWireframe(false);

		const auto meshData = pmdf.createTeapot();
		
		//const auto meshData = pmdf.createSphere(4);
		child1->addComponent<Mesh_data_component>().setMeshData(meshData);
	}

	{
		const auto &child2 = entityManager.instantiate("Primitive Child 2", *root1);
		std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
		material->setBaseColor(Color(Colors::Green));
		child2->addComponent<Renderable_component>().setMaterial(std::unique_ptr<Material>(material.release()));

		const auto meshData = pmdf.createQuad({ 3, 3 });
		child2->addComponent<Mesh_data_component>().setMeshData(meshData);

		child2->setRotation(Quaternion::CreateFromYawPitchRoll(0.0, XM_PI * -0.5f, 0.0));
		child2->setPosition({ 0.0f, -0.5f, 0.0f });
	}

}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	_window = window;
    m_deviceResources->SetWindow(window, width, height);

	m_scene = std::make_unique<Scene_device_resource_aware>(*(m_deviceResources.get()));
	m_scene->initialize();

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
		//LoadGLTF("FlightHelmet", true)->setScale({ 14, 14, 14 });
		//LoadGLTF("WaterBottle", true)->setScale({ 80, 80, 80 });
		
		//CreateSceneMetalRoughSpheres(true);

		CreateCamera(false);
		CreateLights();
		CreateGeometricPrimitives();
	}
	catch (const std::exception &e)
	{
		LOG(FATAL) << "Failed to load scene: " << e.what();
	}

	createDeviceDependentResources();
	createWindowSizeDependentResources();
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
	m_scene->manager<IEntity_render_manager>().render();
	
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

	m_scene->destroyWindowSizeDependentResources();

    createWindowSizeDependentResources();

	const auto mainCamera = m_scene->mainCamera();
	if (mainCamera) {
		auto& entityManager = m_scene->manager<IScene_graph_manager>();
		const auto camera = mainCamera->getFirstComponentOfType<Camera_component>();

		if (g_debugEntity) {
			entityManager.destroy(g_debugEntity->getId());
			g_debugEntity.reset();
		}

		if (camera != nullptr && width > 0 && height > 0) {
			const auto &child3 = entityManager.instantiate("Primitive Child 3");
			std::unique_ptr<Unlit_material> material = std::make_unique<Unlit_material>();
			material->setBaseColor(Color(Colors::White));

			auto& renderable = child3->addComponent<Renderable_component>();
			renderable.setMaterial(std::unique_ptr<Material>(material.release()));

			const auto projMatrixRH = XMMatrixPerspectiveFovRH(
				camera->fov(),
				static_cast<float>(width) / static_cast<float>(height),
				camera->nearPlane(),
				camera->farPlane() * 0.5f);
			
			// DirectX BoundingFrustum's are LH.
			auto frustum = BoundingFrustumRH(projMatrixRH);

			frustum.Origin = mainCamera->worldPosition();
			frustum.Orientation = mainCamera->worldRotation();

			Primitive_mesh_data_factory pmdf;
			const auto meshData = pmdf.createFrustumLines(frustum);
			child3->addComponent<Mesh_data_component>().setMeshData(meshData);

		}
	}
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
	// Get dimensions of the primary monitor
	RECT rc;
	GetWindowRect(GetDesktopWindow(), &rc);

	// Set window to 75% of the desktop size
    width = std::max(320l, rc.right >> 1);
    height = std::max(200l, rc.bottom >> 1);
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
	catch (std::exception &e)
	{
		LOG(FATAL) << "Failed to create device dependent resources: " << e.what();
		m_fatalError = true;
	}
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::createWindowSizeDependentResources()
{
	try
	{
		m_deviceResources->CreateWindowSizeDependentResources();
		const auto outputSize = m_deviceResources->GetOutputSize();
		m_scene->createWindowSizeDependentResources(_window, 
			std::max<UINT>(outputSize.right - outputSize.left, 1),
			std::max<UINT>(outputSize.bottom - outputSize.top, 1));
	}
	catch (std::exception &e)
	{
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
