#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Component.h"
#include "Entity_graph_loader_gltf.h"
#include "Camera_component.h"
#include "Input_manager.h"

#include <type_traits>

using namespace oe;

template<typename TBase, typename TTuple, int TIdx = 0>
constexpr void forEachOfType(TTuple& managers, const std::function<void(TBase*)>& fn)
{
	// Initialize only types that derive from TBase
	if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		fn(std::get<TIdx>(managers).get());

	// iterate the next manager
	if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
		forEachOfType<TBase, TTuple, TIdx + 1>(managers, fn);
}

template<typename TBase, typename TTuple, int TIdx = std::tuple_size_v<TTuple> -1>
constexpr void forEachOfTypeReverse(TTuple& managers, const std::function<void(TBase*)>& fn)
{
	// Initialize only types that derive from TBase
	if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		fn(std::get<TIdx>(managers).get());

	// iterate the next manager (in reverse order than initialized)
	if constexpr (TIdx > 0)
		forEachOfTypeReverse<TBase, TTuple, TIdx - 1>(managers, fn);
}

template<typename TTuple, int TIdx = 0>
constexpr void forEach_tick(TTuple& managers)
{
	// Tick only types that derive from Manager_tickable
	if constexpr (std::is_base_of_v<Manager_tickable, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		std::get<TIdx>(managers)->tick();

	// Recursively iterate to the next tuple index
	if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
		forEach_tick<TTuple, TIdx + 1>(managers);
}

template<typename TTuple, int TIdx = 0>
constexpr void forEach_processMessage(TTuple& managers, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Tick only types that derive from Manager_windowsMessageProcessor
	if constexpr (std::is_base_of_v<Manager_windowsMessageProcessor, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		std::get<TIdx>(managers)->processMessage(message, wParam, lParam);

	// Recursively iterate to the next tuple index
	if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
		forEach_processMessage<TTuple, TIdx + 1>(managers, message, wParam, lParam);
}

void Scene::initialize()
{
	using namespace std;

	// Mesh loaders
	addMeshLoader<Entity_graph_loader_gltf>();

	forEachOfType<Manager_base>(_managers, [](auto* manager) { 
		assert(manager != nullptr);
		manager->initialize(); 
	});
}

void Scene::loadEntities(const std::string& filename)
{
	return loadEntities(filename, nullptr);
}

void Scene::loadEntities(const std::string& filename, Entity& parentEntity)
{
	auto entity = std::get<std::shared_ptr<IEntity_repository>>(_managers)->getEntityPtrById(parentEntity.getId());
	return loadEntities(filename, entity.get());
}

void Scene::loadEntities(const std::string& filename, Entity *parentEntity)
{
	// Get the file extension
	const auto dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos)
		throw std::runtime_error("Cannot load mesh; given file doesn't have an extension.");

	const std::string extension = filename.substr(dotPos + 1);
	const auto extPos = _entityGraphLoaders.find(extension);
	if (extPos == _entityGraphLoaders.end())
		throw std::runtime_error("Cannot load mesh; no registered loader for extension: " + extension);
	
	std::vector<std::shared_ptr<Entity>> newRootEntities = extPos->second->loadFile(
		filename, 
		*std::get<std::shared_ptr<IEntity_repository>>(_managers).get(),
		*std::get<std::shared_ptr<IMaterial_repository>>(_managers).get(),
		*std::get<std::shared_ptr<Primitive_mesh_data_factory>>(_managers).get(),
		true);
	
	if (parentEntity)
	{
		for (const auto& entity : newRootEntities)
			entity->setParent(*parentEntity);
	}

	sceneGraphManager().handleEntitiesLoaded(newRootEntities);
}

void Scene::tick(DX::StepTimer const& timer)
{	
	_deltaTime = timer.GetElapsedSeconds();
	_elapsedTime += _deltaTime;
	
	forEach_tick(_managers);
}

void Scene::shutdown()
{
	forEachOfTypeReverse<Manager_base>(_managers, [](Manager_base* manager) { manager->shutdown(); });

	_managers = decltype(_managers)();
}

void Scene::onComponentAdded(Entity& entity, Component& component) const
{
	sceneGraphManager().handleEntityComponentAdd(entity, component);
}

void Scene::onComponentRemoved(Entity& entity, Component& component) const
{
	sceneGraphManager().handleEntityComponentAdd(entity, component);
}

void Scene::onEntityAdded(Entity& entity) const
{
	sceneGraphManager().handleEntityAdd(entity);
}

void Scene::onEntityRemoved(Entity& entity) const
{
	sceneGraphManager().handleEntityRemove(entity);
}

void Scene::setMainCamera(const std::shared_ptr<Entity>& cameraEntity)
{	
	if (cameraEntity) {
		const auto cameras = cameraEntity->getComponentsOfType<Camera_component>();
		if (cameras.empty())
			throw std::invalid_argument("Given entity must have exactly one CameraComponent");

		_mainCamera = cameraEntity;
	}
	else
		_mainCamera = nullptr;
}


void Scene_device_resource_aware::createWindowSizeDependentResources(HWND window, int width, int height)
{
    LOG(INFO) << "Creating window size dependent resources";
	forEachOfType<Manager_windowDependent>(_managers, [=](Manager_windowDependent* manager) {
		manager->createWindowSizeDependentResources(_deviceResources, window, width, height);
	});
}

void Scene_device_resource_aware::destroyWindowSizeDependentResources()
{
    LOG(INFO) << "Destroying window size dependent resources";
	forEachOfType<Manager_windowDependent>(_managers, [](Manager_windowDependent* manager) {
		manager->destroyWindowSizeDependentResources();
	});
}

void Scene_device_resource_aware::createDeviceDependentResources()
{
    LOG(INFO) << "Creating device dependent resources";
	forEachOfType<Manager_deviceDependent>(_managers, [this](Manager_deviceDependent* manager) {
		manager->createDeviceDependentResources(_deviceResources);
	});
}

void Scene_device_resource_aware::destroyDeviceDependentResources()
{
    LOG(INFO) << "Destroying device dependent resources";
	forEachOfType<Manager_deviceDependent>(_managers, [](Manager_deviceDependent* manager) {
		manager->destroyDeviceDependentResources();
	});
	if (_skyBoxTexture)
		_skyBoxTexture->unload();
}

void Scene_device_resource_aware::processMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
	forEach_processMessage(_managers, message, wParam, lParam);
}

Scene_device_resource_aware::Scene_device_resource_aware(DX::DeviceResources& deviceResources)
	: Scene()
	, _deviceResources(deviceResources)
{
}

void Scene_device_resource_aware::initialize()
{
	using namespace std;

	// Repositories
	const auto entityRepository = make_shared<Entity_repository>(*this);
	const auto materialRepository = make_shared<Material_repository>();
	get<shared_ptr<IEntity_repository>>(_managers) = entityRepository;
	get<shared_ptr<IMaterial_repository>>(_managers) = materialRepository;

	// Factories
	get<shared_ptr<Primitive_mesh_data_factory>>(_managers) = make_shared<Primitive_mesh_data_factory>();

	// Services / Managers
	get<shared_ptr<IScene_graph_manager>>(_managers) = make_shared<Scene_graph_manager>(*this, entityRepository);
	get<shared_ptr<IDev_tools_manager>>(_managers) = make_shared<Dev_tools_manager>(*this);
	get<shared_ptr<ID3D_resources_manager>>(_managers) = make_shared<D3D_resources_manager>(*this, _deviceResources);
	get<shared_ptr<IEntity_render_manager>>(_managers) = make_shared<Entity_render_manager>(*this, materialRepository);
	get<shared_ptr<IRender_step_manager>>(_managers) = make_shared<Render_step_manager>(*this);
	get<shared_ptr<IShadowmap_manager>>(_managers) = make_shared<Shadowmap_manager>(*this, _deviceResources);
	get<shared_ptr<IEntity_scripting_manager>>(_managers) = make_shared<Entity_scripting_manager>(*this);
	get<shared_ptr<IAsset_manager>>(_managers) = make_shared<Asset_manager>(*this);
	get<shared_ptr<IInput_manager>>(_managers) = make_shared<Input_manager>(*this, _deviceResources);

	Scene::initialize();
}
