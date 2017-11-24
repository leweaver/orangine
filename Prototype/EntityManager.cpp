#include "pch.h"
#include "EntityManager.h"
#include "Entity.h"

using namespace OE;

void OE::EntityManager::Tick(float elapsedTime)
{
	m_deltaTime = elapsedTime - m_elapsedTime;
	m_elapsedTime = elapsedTime;

	for (auto const& entity : m_rootEntities)
	{
		if (!entity.second->IsActive())
			continue;

		entity.second->Update();
	}
}

Entity& EntityManager::Instantiate(std::string name)
{
	Entity* entity = new Entity(*this, name, ++lastEntityId);
	m_rootEntities[entity->m_id] = std::unique_ptr<Entity>(entity);
	return *m_rootEntities[entity->m_id].get();
}

Entity& EntityManager::Instantiate(std::string name, Entity& parent)
{
	Entity& entity = Instantiate(name);
	entity.SetParent(parent);
	return entity;
}

std::unique_ptr<Entity> EntityManager::TakeRoot(Entity& entity)
{
	std::unique_ptr<Entity> &root = m_rootEntities[entity.m_id];
	std::unique_ptr<Entity> newRoot = move(root);
	m_rootEntities.erase(entity.m_id);
	return newRoot;
}

void EntityManager::PutRoot(std::unique_ptr<Entity>& entity)
{
	m_rootEntities[entity.get()->m_id] = move(entity);
}
