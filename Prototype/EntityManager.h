#pragma once
#include "Entity.h"
#include <vector>
#include <map>

namespace OE {

class EntityManager
{
	friend Entity;

	Scene& m_scene;

	// A cache of entities that have no parents
	std::vector<std::weak_ptr<Entity>> m_rootEntities;

	// All entities
	Entity::EntityPtrMap m_entities;
	
	// Values as of the last call to Tick.
	double m_deltaTime = 0;
	double m_elapsedTime = 0;
	unsigned int lastEntityId = 0;

	bool m_initialized = false;

public:
	
	explicit EntityManager(Scene& scene);
	EntityManager(const EntityManager& other) = delete;

	/** Total time since game start, in seconds. */
	const double& ElapsedTime() const
	{
		return m_elapsedTime;
	}

	/** Time since last frame, in seconds. */
	const double& DeltaTime() const
	{
		return m_deltaTime;
	}

	void Tick(double elapsedTime);

	Entity& Instantiate(std::string name);
	Entity& Instantiate(std::string name, Entity& parent);

	void Destroy(Entity& entity);
	void Destroy(const Entity::ID_TYPE& entity);

private:
	std::shared_ptr<Entity> RemoveFromRoot(const Entity& entity);
	void AddToRoot(const Entity& entity);
};

}
