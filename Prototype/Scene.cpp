#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Component.h"
#include "Entity_graph_loader_gltf.h"
#include "Camera_component.h"

using namespace oe;

Scene::Scene(DX::DeviceResources& deviceResources)
{
	// Mesh loaders
	AddMeshLoader<Entity_graph_loader_gltf>();

	// Repositories
	_entityRepository = std::make_shared<Entity_repository>(*this);
	_materialRepository = std::make_shared<Material_repository>();

	// Factories
	_primitiveMeshDataFactory = std::make_shared<Primitive_mesh_data_factory>();

	// Services / Managers
	_sceneGraphManager = std::make_unique<Scene_graph_manager>(*this, _entityRepository);
	_entityRenderManager = std::make_unique<Entity_render_manager>(*this, _materialRepository, deviceResources);
	_entityScriptinigManager = std::make_unique<Entity_scripting_manager>(*this);
	_assetManager = std::make_unique<Asset_manager>(*this);

	_sceneGraphManager->initialize();
	_entityRenderManager->initialize();
	_entityScriptinigManager->initialize();
	_assetManager->initialize();
}

void Scene::loadEntities(const std::string& filename)
{
	return loadEntities(filename, nullptr);
}


void Scene::loadEntities(const std::string& filename, Entity& parentEntity)
{
	auto entity = _entityRepository->getEntityPtrById(parentEntity.getId());
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
		*_entityRepository, 
		*_materialRepository,
		*_primitiveMeshDataFactory);
	
	if (parentEntity)
	{
		for (const auto& entity : newRootEntities)
			entity->setParent(*parentEntity);
	}

	_sceneGraphManager->handleEntitiesLoaded(newRootEntities);
}

void Scene::tick(DX::StepTimer const& timer)
{	
	_deltaTime = timer.GetElapsedSeconds();
	_elapsedTime += _deltaTime;

	_sceneGraphManager->tick();
	_entityRenderManager->tick();
	_entityScriptinigManager->tick();
}

void Scene::shutdown()
{
	_entityRenderManager->shutdown();
	_entityScriptinigManager->shutdown();
	_sceneGraphManager->shutdown();
	
	_assetManager.reset();
	_entityScriptinigManager.reset();
	_entityRenderManager.reset();
	_sceneGraphManager.reset();
	
	_entityRepository.reset();
	_materialRepository.reset();
}

void Scene::onComponentAdded(Entity& entity, Component& component) const
{
	_sceneGraphManager->handleEntityComponentAdd(entity, component);
}

void Scene::onComponentRemoved(Entity& entity, Component& component) const
{
	_sceneGraphManager->handleEntityComponentAdd(entity, component);
}

void Scene::onEntityAdded(Entity& entity) const
{
	_sceneGraphManager->handleEntityAdd(entity);
}

void Scene::onEntityRemoved(Entity& entity) const
{
	_sceneGraphManager->handleEntityRemove(entity);
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
