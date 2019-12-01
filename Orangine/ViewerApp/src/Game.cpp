//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

#include "ViewerAppConfig.h"

#include <OeCore/IScene_graph_manager.h>
#include <OeCore/Scene.h>
#include <OeCore/Test_component.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/Camera_component.h>
#include <OeCore/Light_component.h>
#include <OeCore/Renderable_component.h>
#include <OeCore/PBR_material.h>
#include <OeCore/Collision.h>
#include <OeScripting/Script_component.h>
#include <OeCore/Color.h>

#include <filesystem>


extern void ExitGame();

using namespace DirectX;
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
		m_scene->destroyWindowSizeDependentResources();
		m_scene->destroyDeviceDependentResources();
		m_scene->shutdown();
	}

	ComPtr<ID3D11Device> device = m_deviceResources->GetD3DDevice();
	ComPtr<ID3D11Debug> d3DDebug;
	if (device && SUCCEEDED(device.As(&d3DDebug))) {
		//d3DDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}

	m_deviceResources.reset();
}

void Game::CreateSceneCubeSatellite()
{
	auto& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto root1 = entityManager.instantiate("Root 1");
	//root1->AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.1250f, 0.06f, 0.0f));

	const auto child1 = entityManager.instantiate("Child 1", *root1);
	const auto child2 = entityManager.instantiate("Child 2", *root1);
	const auto child3 = entityManager.instantiate("Child 3", *root1);

	AddCubeToEntity(child1, {0.5f, 0.0f, 0.0f}, {2, 1, 1}, { 4, 0, 0});
	AddCubeToEntity(child2, {0.0f, 0.5f, 0.0f}, {1, 2, 1}, { 0, 0, 0});
	AddCubeToEntity(child3, {0.0f, 0.0f, 0.5f}, {1, 1, 2}, {-4, 0, 0});
}

std::shared_ptr<Entity> Game::LoadGLTF(std::string gltfName, bool animate)
{
	auto& entityManager = m_scene->manager<IScene_graph_manager>();
	auto root = entityManager.instantiate("Root");

	if (animate)
		root->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });

	return LoadGLTFToEntity(gltfName, root);
}



std::shared_ptr<Entity> Game::LoadGLTFToEntity(std::string gltfName, std::shared_ptr<Entity> root) {
    const auto gltfPathSubfolder = "/" + gltfName + "/glTF/" + gltfName + ".gltf";
    auto gltfPath = m_scene->manager<IAsset_manager>().makeAbsoluteAssetPath(
            utf8_decode("ViewerApp/data/meshes" + gltfPathSubfolder));
    LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);
    
    if (!std::filesystem::exists(gltfPath)) {
        std::wstringstream ss;
        ss  << VIEWERAPP_THIRDPARTY_PATH 
            << utf8_decode("/glTF-Sample-Models/2.0")
            << utf8_decode(gltfPathSubfolder);
        gltfPath = ss.str();
        
        LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);
        if (!std::filesystem::exists(gltfPath)) {
            throw std::runtime_error("Could not find gltf file with name: " + gltfName);
        }
    }

    m_scene->loadEntities(gltfPath, *root);

	return root;
}

void Game::CreateCamera(bool animate)
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();

	auto cameraDollyAnchor = entityManager.instantiate("CameraDollyAnchor");
	if (animate)
		cameraDollyAnchor->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});
	
	auto camera = entityManager.instantiate("Camera", *cameraDollyAnchor);
	camera->setPosition(SimpleMath::Vector3(0.0f, 0.0f, 15));

	cameraDollyAnchor->computeWorldTransform();

	auto& component = camera->addComponent<Camera_component>();
	component.setFov(XMConvertToRadians(60.0f));
	component.setFarPlane(20.0f);
	component.setNearPlane(0.5f);

	camera->lookAt({ 0, 0, 0 }, SimpleMath::Vector3::Up);

	m_scene->setMainCamera(camera);
}

void Game::CreateLights()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	int lightCount = 0;
	auto createDirLight = [&entityManager, &lightCount](const SimpleMath::Vector3& normal, const Color& color, float intensity)
	{
		auto lightEntity = entityManager.instantiate("Directional Light " + std::to_string(++lightCount));
		auto& component = lightEntity->addComponent<Directional_light_component>();
		component.setColor(color);
		component.setIntensity(intensity);

		if (normal != SimpleMath::Vector3::Forward)
		{
			SimpleMath::Vector3 axis;
			if (normal == SimpleMath::Vector3::Backward)
				axis = SimpleMath::Vector3::Up;
			else
			{
				axis = SimpleMath::Vector3::Forward.Cross(normal);
				axis.Normalize();
			}

			assert(normal.LengthSquared() != 0);
			float angle = acos(SimpleMath::Vector3::Forward.Dot(normal) / normal.Length());
			lightEntity->setRotation(SimpleMath::Quaternion::CreateFromAxisAngle(axis, angle));
		}
		return lightEntity;
	};
	auto createPointLight = [&entityManager, &lightCount](const SimpleMath::Vector3 &position, const Color &color, float intensity)
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
	
	if (true)
	{
		auto shadowLight1 = createDirLight({ 0.0f, -1.0f, 0.0f }, { 1, 1, 1, 1 }, 2);
		shadowLight1->setParent(*lightRoot);
		shadowLight1->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(false);
		
		
		auto shadowLight2 = createDirLight({ -0.707f, -0.707f, -0.707f }, { 1, 1, 0, 1 }, 2.75);
		shadowLight2->setParent(*lightRoot);
		shadowLight2->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(true);

		createDirLight({ -0.666f, -0.333f, 0.666f }, { 1, 0, 1, 1 }, 4.0)->setParent(*lightRoot);
		
	}
	else
	{
		createPointLight({ 10, 0, 10 }, { 1, 1, 1, 1 }, 2*13)->setParent(*lightRoot);
		createPointLight({ 10, 5, -10 }, { 1, 0, 1, 1 }, 2*20)->setParent(*lightRoot);
	}

	//createAmbientLight({ 1, 1, 1 }, 0.2f)->setParent(*lightRoot);
}

void Game::CreateSceneLeverArm()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto root1 = entityManager.instantiate("Root 1");
	root1->setPosition({ 5, 0, 5 });

	const auto child1 = entityManager.instantiate("Child 1", *root1);
	const auto child2 = entityManager.instantiate("Child 2", *child1);
	const auto child3 = entityManager.instantiate("Child 3", *child2);

	AddCubeToEntity(child1, { 0, 0, 0.1f }, { 0.5f, 0.5f, 1.0f }, { 0, 0, 0 });
	AddCubeToEntity(child2, { 0, 0.25, 0 }, { 1.0f, 2.0f, 0.5f }, { 0, 0, 2 });
	AddCubeToEntity(child3, { 0.5f, 0, 0 }, { 2.0f, 0.50f, 1.0f }, { 0, 2, 0});
}

void Game::CreateScripts() {

	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto& root1 = entityManager.instantiate("Script Container");

	auto& scriptComponent = root1->addComponent<Script_component>();
	scriptComponent.setScriptName("testmodule.TestComponent");
}

void Game::CreateShadowTestScene()
{
	IScene_graph_manager& entityManager = m_scene->manager<IScene_graph_manager>();
	const auto &root1 = entityManager.instantiate("Primitives");
	int teapotCount = 0;

	auto createTeapot = [&entityManager, &root1, &teapotCount](const SimpleMath::Vector3& center, const Color& color, float metallic, float roughness)
	{
		const auto &child1 = entityManager.instantiate("Teapot " + std::to_string(++teapotCount), *root1);
		child1->setPosition(center);

		std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
		material->setBaseColor(color);
		material->setMetallicFactor(metallic);
		material->setRoughnessFactor(roughness);
		
		auto& renderable = child1->addComponent<Renderable_component>();
		renderable.setMaterial(std::unique_ptr<Material>(material.release()));
		renderable.setWireframe(false);
		renderable.setCastShadow(true);

		const auto meshData = Primitive_mesh_data_factory::createTeapot();
		child1->addComponent<Mesh_data_component>().setMeshData(meshData);
		child1->setBoundSphere(BoundingSphere(SimpleMath::Vector3::Zero, 1.0f));
		child1->addComponent<Test_component>().setSpeed({ 0.0f, 0.1f, 0.0f });
	};
	auto createSphere = [&entityManager, &root1](const SimpleMath::Vector3& center, const Color& color, float metallic, float roughness)
	{
		const auto &child1 = entityManager.instantiate("Primitive Child 1", *root1);
		child1->setPosition(center);

		std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
		material->setBaseColor(color);
		material->setMetallicFactor(metallic);
		material->setRoughnessFactor(roughness);

		auto& renderable = child1->addComponent<Renderable_component>();
		renderable.setMaterial(std::unique_ptr<Material>(material.release()));
		renderable.setWireframe(false);
        renderable.setCastShadow(false);

		const auto meshData = Primitive_mesh_data_factory::createSphere();
		child1->addComponent<Mesh_data_component>().setMeshData(meshData);
		child1->setBoundSphere(BoundingSphere(SimpleMath::Vector3::Zero, 1.0f));
	};

    int created = 0;
    //for (float metallic = 0.0f; metallic <= 1.0f; metallic += 0.2f) {
    //    for (float roughness = 0.0f; roughness <= 1.0f; roughness += 0.2f) {
    //        createSphere({ metallic * 10.0f - 5.0f,  roughness * 10.0f - 5.0f, 0 }, Color(Colors::White), 1.0f - metallic, 1.0f - roughness);
    //        created++;
    //    }
    //}

	if (true)
	{
		// Create the floor
		const auto &child2 = entityManager.instantiate("Primitive Child 2", *root1);
		auto material = std::make_unique<PBR_material>();
		material->setBaseColor(Color(0.7f, 0.7f, 0.7f, 1.0f));

		auto& renderable = child2->addComponent<Renderable_component>();
		renderable.setMaterial(std::unique_ptr<Material>(material.release()));
		renderable.setCastShadow(false);

		const auto meshData = Primitive_mesh_data_factory::createQuad(20, 20);
		child2->addComponent<Mesh_data_component>().setMeshData(meshData);

		child2->setRotation(SimpleMath::Quaternion::CreateFromYawPitchRoll(0.0, XM_PI * -0.5f, 0.0));
		child2->setPosition({ 0.0f, -1.5f, 0.0f });
		child2->setBoundSphere(BoundingSphere(SimpleMath::Vector3::Zero, 10.0f));
	}

	createTeapot({ -2, 0, -2 }, oe::Colors::Green, 1.0, 0.0f);
	createTeapot({  2, 0, -2 }, oe::Colors::Red,   1.0, 0.25f);
	createTeapot({ -2, 0,  2 }, oe::Colors::White, 1.0, 0.75f);
	createTeapot({  2, 0,  2 }, oe::Colors::Black, 1.0, 0.0f);


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
		//LoadGLTF("FlightHelmet", false)->setScale({ 7, 7, 7 });
		//LoadGLTF("WaterBottle", true)->setScale({ 40, 40, 40 });
        //LoadGLTF("InterpolationTest", false);
        //LoadGLTF("MorphPrimitivesTest", false)->setPosition({0, -3.0f, 0});
        //LoadGLTF("AnimatedMorphCube", false)->setPosition({ 0, -3.0f, 0 });
        //LoadGLTF("Alien", false)->setScale({ 10.01f, 10.01f, 10.01f });
        //LoadGLTF("MorphCube2", false);
        //LoadGLTF("RiggedSimple", false);
        //LoadGLTF("RiggedFigure", false);
		//LoadGLTF("CesiumMan", false)->setPosition({ 0, 0, 1.0f });
        //LoadGLTF("WaterBottle", true)->setScale({ 40, 40, 40 });

		//LoadGLTF("MetalRoughSpheres", false);

		CreateCamera(false);
		CreateLights();
		CreateShadowTestScene();
		CreateScripts();



		// Load the skybox
		auto skyBoxTexture = std::make_shared<File_texture>(
                m_scene->manager<IAsset_manager>().makeAbsoluteAssetPath(L"ViewerApp/textures/park-cubemap.dds")
            );
		m_scene->setSkyboxTexture(skyBoxTexture);
	}
	catch (const std::exception &e)
	{
		LOG(WARNING) << "Failed to load scene: " << e.what();
		m_fatalError = true;
		return;
	}

	try {
		createDeviceDependentResources();
		createWindowSizeDependentResources();
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Failed to create device resources: " << e.what();
		m_fatalError = true;
		return;
	}
}

void Game::AddCubeToEntity(std::shared_ptr<Entity> entity, SSE::Vector3 animationSpeed, SimpleMath::Vector3 localScale, SimpleMath::Vector3 localPosition)
{
	entity->addComponent<Test_component>().setSpeed(animationSpeed);

	entity->setScale(localScale);
	entity->setPosition(localPosition);
	
	auto child = LoadGLTFToEntity("Cube", entity);
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
	//const auto mouseState = m_scene->manager<IInput_manager>().mouseState().lock();
	//if (IInput_manager::Mouse_state::Button_state::RELEASED == mouseState->right) {
    if (m_scene->mainCamera()) {
        m_scene->manager<IRender_step_manager>().render(m_scene->mainCamera());
    }
    
    m_scene->manager<IUser_interface_manager>().render();

	// Show the new frame.
	m_deviceResources->Present();
	//}
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
