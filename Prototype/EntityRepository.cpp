#include "pch.h"
#include "EntityRepository.h"
#include "Entity.h"

using namespace OE;

EntityRepository::EntityRepository(Scene& scene)
	: lastEntityId(0)
	, m_scene(scene)
{
}

std::shared_ptr<Entity> EntityRepository::Instantiate(const std::string& name)
{
	Entity *entity = new Entity(this->m_scene, name, ++lastEntityId);
	auto entityPtr = std::shared_ptr<Entity>(entity);
	m_entities[lastEntityId] = entityPtr;
	return entityPtr;
}

void EntityRepository::Remove(const Entity::ID_TYPE id)
{
	const auto pos = m_entities.find(id);
	if (pos != m_entities.end()) {
		m_entities.erase(pos);
	}
}

std::shared_ptr<Entity> EntityRepository::GetEntityPtrById(const Entity::ID_TYPE id)
{
	const auto pos = m_entities.find(id);
	if (pos == m_entities.end())
		return nullptr;
	return pos->second;
}

Entity &EntityRepository::GetEntityById(const Entity::ID_TYPE id)
{
	auto pos = m_entities.find(id);
	assert(pos != m_entities.end() && "Invalid Entity ID provided");
	return *pos->second.get();
}