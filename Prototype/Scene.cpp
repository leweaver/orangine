#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Component.h"
#include "EntityRenderService.h"
#include "EntityManager.h"
#include "RenderableComponent.h"

using namespace OE;

Scene::Scene()
{
	m_entityManager = std::make_unique<OE::EntityManager>(*this);
	m_entityRenderer = std::make_unique<OE::EntityRenderService>(*this);

	m_entityRenderer->Initialize();
}

Scene::~Scene()
{
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
