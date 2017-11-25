#pragma once
#include "Entity.h"
#include <vector>
#include <map>

namespace OE {

class EntityManager
{
	friend Entity;

	// A cache of entities that have no parents
	std::vector<std::weak_ptr<Entity>> m_rootEntities;

	// All entities
	Entity::EntityPtrMap m_entities;
	
	// Values as of the last call to Tick.
	float m_deltaTime = 0;
	float m_elapsedTime = 0;
	unsigned int lastEntityId = 0;

public:
	
	EntityManager() = default;
	EntityManager(const EntityManager& other) = delete;

	const float& MElapsedTime() const
	{
		return m_elapsedTime;
	}

	const float& MDeltaTime() const
	{
		return m_deltaTime;
	}

	void Tick(float elapsedTime);

	Entity& Instantiate(std::string name);
	Entity& Instantiate(std::string name, Entity& parent);

	void Destroy(Entity& entity);
	void Destroy(const Entity::ID_TYPE& entity);

private:
	std::shared_ptr<Entity> RemoveFromRoot(const Entity& entity);
	void AddToRoot(const Entity& entity);
};

}
