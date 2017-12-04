﻿#pragma once
#include "Entity.h"
#include "Component.h"
#include "EntityFilter.h"

#include <vector>
#include <map>
#include <set>
#include <typeinfo>

namespace OE {

class EntityManager
{
	friend Entity;
	class EntityFilterImpl;

	Scene& m_scene;

	// A cache of entities that have no parents
	std::vector<std::weak_ptr<Entity>> m_rootEntities;
	std::vector<std::shared_ptr<EntityFilterImpl>> m_entityFilters;

	using ComponentTypeSet = std::set<Component::ComponentType>;

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

	std::shared_ptr<EntityFilter> GetEntityFilter(const ComponentTypeSet &componentTypes);

	void HandleEntityAdd(const Entity &entity);
	void HandleEntityRemove(const Entity &entity);
	void HandleEntityComponentAdd(const Entity &entity, const Component &componentType);
	void HandleEntityComponentRemove(const Entity &entity, const Component &componentType);

private:
	std::shared_ptr<Entity> RemoveFromRoot(const Entity& entity);
	void AddToRoot(const Entity& entity);

	class EntityFilterImpl : public EntityFilter {
	public:
		std::set<Component::ComponentType> m_componentTypes; 
		
		EntityFilterImpl(const ComponentTypeSet::const_iterator &begin, const ComponentTypeSet::const_iterator &end);

		void HandleEntityAdd(const std::shared_ptr<Entity> &entity);
		void HandleEntityRemove(const std::shared_ptr<Entity> &entity);
		void HandleEntityComponentsUpdated(const std::shared_ptr<Entity> &entity);

	};
};

}
