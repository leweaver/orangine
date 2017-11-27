#include "pch.h"
#include "Scene.h"
#include "RenderableComponent.h"

using namespace OE;

Scene::Scene()
{
	m_entityManager = std::make_unique<OE::EntityManager>(*this);
	m_entityRenderer = std::make_unique<OE::EntityRenderer>(*this);
}

Scene::~Scene()
{
}

void Scene::OnComponentAdded(Entity& entity, Component& component) const
{
	const auto renderableComponent = dynamic_cast<RenderableComponent*>(&component);
	if (renderableComponent != nullptr)
	{
		m_entityRenderer->AddRenderable(*renderableComponent);		
	}
}

void Scene::OnComponentRemoved(Entity& entity, Component& component) const
{
	const auto renderableComponent = dynamic_cast<RenderableComponent*>(&component);
	if (renderableComponent != nullptr)
	{
		m_entityRenderer->RemoveRenderable(*renderableComponent);
	}
}

void Scene::OnEntityAdded(Entity& entity) const
{
}

void Scene::OnEntityRemoved(Entity& entity) const
{
	//m_entityRenderer
}
