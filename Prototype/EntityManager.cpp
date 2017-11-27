#include "pch.h"
#include "EntityManager.h"
#include "Entity.h"

using namespace OE;

EntityManager::EntityManager(Scene& scene)
	: m_scene(scene)
{
}

void EntityManager::Tick(float elapsedTime)
{
	if (!m_initialized)
	{
		for (auto const& weakPtr : m_rootEntities)
		{
			std::shared_ptr<Entity> entity = weakPtr.lock();
			if (entity == nullptr) {
				assert(false);
				continue;
			}

			entity->Initialize();
		}
		m_initialized = true;
	}

	m_deltaTime = elapsedTime - m_elapsedTime;
	m_elapsedTime = elapsedTime;

	for (auto const& weakPtr : m_rootEntities)
	{
		std::shared_ptr<Entity> entity = weakPtr.lock();
		if (entity == nullptr) {
			assert(false);
			continue;
		}

		if (!entity->IsActive())
			continue;

		entity->Update();
	}
}

Entity& EntityManager::Instantiate(std::string name)
{
	Entity* entity = new Entity(this->m_scene, name, ++lastEntityId);
	const auto entityPtr = std::shared_ptr<Entity>(entity);
	m_entities[entity->GetId()] = entityPtr;
	m_rootEntities.push_back(std::weak_ptr<Entity>(entityPtr));

	if (m_initialized)
		entity->Initialize();

	return *entity;
}

Entity& EntityManager::Instantiate(std::string name, Entity& parent)
{
	Entity& entity = Instantiate(name);
	entity.SetParent(parent);
	return entity;
}

void EntityManager::Destroy(Entity& entity)
{
	entity.RemoveParent();
	RemoveFromRoot(entity);

	// This will delete the object. Make sure it is the last operation!
	m_entities.erase(entity.GetId());
}

void EntityManager::Destroy(const Entity::ID_TYPE& entityId)
{
	auto& entityPtr = m_entities[entityId];
	const auto entity = entityPtr.get();
	if (entity != nullptr) {
		Destroy(*entity);
	}
}

std::shared_ptr<Entity> EntityManager::RemoveFromRoot(const Entity& entity)
{
	const std::shared_ptr<Entity> entityPtr = m_entities[entity.GetId()];

	// remove from root array
	for (auto rootIter = m_rootEntities.begin(); rootIter != m_rootEntities.end(); ++rootIter) {
		const auto rootEntity = (*rootIter).lock();
		if (rootEntity != nullptr && rootEntity->GetId() == entity.GetId()) {
			m_rootEntities.erase(rootIter);
			break;
		}
	}

	return entityPtr;
}

void EntityManager::AddToRoot(const Entity& entity)
{
	m_rootEntities.push_back(m_entities[entity.GetId()]);
}
