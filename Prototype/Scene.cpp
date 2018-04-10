#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Component.h"
#include "EntityRenderManager.h"
#include "SceneGraphManager.h"
#include "EntityScriptingManager.h"
#include "AssetManager.h"
#include "glTFMeshLoader.h"
#include "CameraComponent.h"

using namespace OE;

Scene::Scene(DX::DeviceResources &deviceResources)
{
	// Mesh loaders
	AddMeshLoader<glTFMeshLoader>();

	// Repositories
	m_entityRepository = std::make_shared<EntityRepository>(*this);
	m_materialRepository = std::make_shared<MaterialRepository>();

	// Factories
	m_primitiveMeshDataFactory = std::make_shared<PrimitiveMeshDataFactory>();

	// Services / Managers
	m_sceneGraphManager = std::make_unique<SceneGraphManager>(*this, m_entityRepository);
	m_entityRenderManager = std::make_unique<EntityRenderManager>(*this, m_materialRepository, deviceResources);
	m_entityScriptinigManager = std::make_unique<EntityScriptingManager>(*this);
	m_assetManager = std::make_unique<AssetManager>(*this);

	m_sceneGraphManager->Initialize();
	m_entityRenderManager->Initialize();
	m_entityScriptinigManager->Initialize();
	m_assetManager->Initialize();
}

Scene::~Scene()
{
}

void Scene::LoadEntities(const std::string& filename)
{
	return LoadEntities(filename, nullptr);
}


void Scene::LoadEntities(const std::string& filename, Entity &parentEntity)
{
	auto entity = m_entityRepository->GetEntityPtrById(parentEntity.GetId());
	return LoadEntities(filename, entity.get());
}

void Scene::LoadEntities(const std::string& filename, Entity *parentEntity)
{
	// Get the file extension
	const auto dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos)
		throw std::runtime_error("Cannot load mesh; given file doesn't have an extension.");

	const std::string extension = filename.substr(dotPos + 1);
	const auto extPos = m_entityGraphLoaders.find(extension);
	if (extPos == m_entityGraphLoaders.end())
		throw std::runtime_error("Cannot load mesh; no registered loader for extension: " + extension);

	std::vector<std::shared_ptr<Entity>> newRootEntities = extPos->second->LoadFile(
		filename, 
		*m_entityRepository, 
		*m_materialRepository,
		*m_primitiveMeshDataFactory);
	
	if (parentEntity)
	{
		for (const auto &entity : newRootEntities)
			entity->SetParent(*parentEntity);
	}

	m_sceneGraphManager->HandleEntitiesLoaded(newRootEntities);
}

void Scene::Tick(DX::StepTimer const& timer)
{	
	m_deltaTime = timer.GetElapsedSeconds();
	m_elapsedTime += m_deltaTime;

	m_sceneGraphManager->Tick();
	m_entityRenderManager->Tick();
	m_entityScriptinigManager->Tick();
}

void Scene::Shutdown()
{
	m_entityRenderManager->Shutdown();
	m_entityScriptinigManager->Shutdown();
	m_sceneGraphManager->Shutdown();
	
	m_assetManager.reset();
	m_entityScriptinigManager.reset();
	m_entityRenderManager.reset();
	m_sceneGraphManager.reset();
	
	m_entityRepository.reset();
	m_materialRepository.reset();
}

void Scene::OnComponentAdded(Entity& entity, Component& component) const
{
	m_sceneGraphManager->HandleEntityComponentAdd(entity, component);
}

void Scene::OnComponentRemoved(Entity& entity, Component& component) const
{
	m_sceneGraphManager->HandleEntityComponentAdd(entity, component);
}

void Scene::OnEntityAdded(Entity& entity) const
{
	m_sceneGraphManager->HandleEntityAdd(entity);
}

void Scene::OnEntityRemoved(Entity& entity) const
{
	m_sceneGraphManager->HandleEntityRemove(entity);
}

void Scene::SetMainCamera(const std::shared_ptr<Entity> &cameraEntity)
{	
	if (cameraEntity) {
		const auto cameras = cameraEntity->GetComponentsOfType<CameraComponent>();
		if (cameras.empty())
			throw std::invalid_argument("Given entity must have exactly one CameraComponent");

		m_mainCamera = cameraEntity;
	}
	else
		m_mainCamera = nullptr;
}
