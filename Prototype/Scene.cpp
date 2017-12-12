#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Component.h"
#include "EntityRenderManager.h"
#include "SceneGraphManager.h"
#include "EntityScriptingManager.h"
#include "AssetManager.h"

using namespace OE;

Scene::Scene()
{
	m_entityManager = std::make_unique<SceneGraphManager>(*this);
	m_entityRenderer = std::make_unique<EntityRenderManager>(*this);
	m_entityScriptinigManager = std::make_unique<EntityScriptingManager>(*this);
	m_assetManager = std::make_unique<AssetManager>(*this);

	m_entityManager->Initialize();
	m_entityRenderer->Initialize();
	m_entityScriptinigManager->Initialize();
	m_assetManager->Initialize();
}

Scene::~Scene()
{
}

void Scene::Tick(DX::StepTimer const& timer)
{	
	m_deltaTime = timer.GetElapsedSeconds();
	m_elapsedTime += m_deltaTime;

	m_entityManager->Tick();
	m_entityRenderer->Tick();
	m_entityScriptinigManager->Tick();
}

void Scene::OnComponentAdded(Entity& entity, Component& component) const
{
	m_entityManager->HandleEntityComponentAdd(entity, component);
}

void Scene::OnComponentRemoved(Entity& entity, Component& component) const
{
	m_entityManager->HandleEntityComponentAdd(entity, component);
}

void Scene::OnEntityAdded(Entity& entity) const
{
	m_entityManager->HandleEntityAdd(entity);
}

void Scene::OnEntityRemoved(Entity& entity) const
{
	m_entityManager->HandleEntityRemove(entity);
}
