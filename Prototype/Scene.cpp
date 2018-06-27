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
	// Initialize only types that derive from Manager_base
	if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		fn(std::get<TIdx>(managers).get());

	// iterate the next manager
	if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
		forEachOfType<TBase, TTuple, TIdx + 1>(managers, fn);
}

template<typename TBase, typename TTuple, int TIdx = std::tuple_size_v<TTuple> -1>
constexpr void forEachOfTypeReverse(TTuple& managers, const std::function<void(TBase*)>& fn)
{
	// Initialize only types that derive from Manager_base
	if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		fn(std::get<TIdx>(managers).get());

	// iterate the next manager (in reverse order than initialized)
	if constexpr (TIdx > 0)
		forEachOfTypeReverse<TBase, TTuple, TIdx - 1>(managers, fn);
}

template<typename TTuple, int TIdx = 0>
constexpr void tickManagers(TTuple& managers)
{
	// Tick only types that derive from Manager_base
	if constexpr (std::is_base_of_v<Manager_tickable, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
		std::get<TIdx>(managers)->tick();

	// Recursively iterate to the next tuple index
	if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
		tickManagers<TTuple, TIdx + 1>(managers);
}

Scene::Scene(DX::DeviceResources& deviceResources)
{
	using namespace std;

	// Mesh loaders
	addMeshLoader<Entity_graph_loader_gltf>();

	// Repositories
	get<shared_ptr<Entity_repository>>(_managers) = make_shared<Entity_repository>(*this);
	get<shared_ptr<Material_repository>>(_managers) = make_shared<Material_repository>();

	// Factories
	get<shared_ptr<Primitive_mesh_data_factory>>(_managers) = make_shared<Primitive_mesh_data_factory>();

	// Services / Managers
	get<unique_ptr<Scene_graph_manager>>(_managers) = make_unique<Scene_graph_manager>(*this, get<shared_ptr<Entity_repository>>(_managers));
	get<unique_ptr<Entity_render_manager>>(_managers) = make_unique<Entity_render_manager>(*this, get<shared_ptr<Material_repository>>(_managers), deviceResources);
	get<unique_ptr<Entity_scripting_manager>>(_managers) = make_unique<Entity_scripting_manager>(*this);
	get<unique_ptr<Asset_manager>>(_managers) = make_unique<Asset_manager>(*this);
	get<unique_ptr<Input_manager>>(_managers) = make_unique<Input_manager>(*this, deviceResources);

	static_assert(std::is_base_of_v<Manager_base, remove_pointer_t<decltype(std::get<unique_ptr<Entity_render_manager>>(_managers).get())>>, "Is a Manager_base!");
	static_assert(std::is_base_of_v<Manager_base, remove_pointer_t<decltype(std::get<3>(_managers).get())>>, "Is a Manager_base!");

	forEachOfType<Manager_base>(_managers, [](auto* manager) { manager->initialize(); });
}

void Scene::loadEntities(const std::string& filename)
{
	return loadEntities(filename, nullptr);
}

void Scene::loadEntities(const std::string& filename, Entity& parentEntity)
{
	auto entity = std::get<std::shared_ptr<Entity_repository>>(_managers)->getEntityPtrById(parentEntity.getId());
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
		*std::get<std::shared_ptr<Entity_repository>>(_managers).get(),
		*std::get<std::shared_ptr<Material_repository>>(_managers).get(),
		*std::get<std::shared_ptr<Primitive_mesh_data_factory>>(_managers).get());
	
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
	
	tickManagers(_managers);
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

void Scene::createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height)
{
	forEachOfType<Manager_windowDependent>(_managers, [&](Manager_windowDependent* manager) {
		manager->createWindowSizeDependentResources(deviceResources, window, width, height);
	});
}

void Scene::destroyWindowSizeDependentResources()
{
	forEachOfType<Manager_windowDependent>(_managers, [](Manager_windowDependent* manager) {
		manager->destroyWindowSizeDependentResources();
	});
}

void Scene::createDeviceDependentResources(DX::DeviceResources& deviceResources)
{
	forEachOfType<Manager_deviceDependent>(_managers, [&deviceResources](Manager_deviceDependent* manager) {
		manager->createDeviceDependentResources(deviceResources);
	});
}

void Scene::destroyDeviceDependentResources()
{
	forEachOfType<Manager_deviceDependent>(_managers, [](Manager_deviceDependent* manager) {
		manager->destroyDeviceDependentResources();
	});
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
