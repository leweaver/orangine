﻿#pragma once
#include "Entity.h"
#include "Component.h"
#include "EntityFilter.h"
#include "ManagerBase.h"
#include "EntityRepository.h"

#include <vector>
#include <set>

namespace OE {
	
enum class EntityFilterMode
{
	ALL,
	ANY
};


class SceneGraphManager : public ManagerBase
{
	friend Entity;
	friend EntityRef;
	class EntityFilterImpl;

	std::vector<std::shared_ptr<EntityFilterImpl>> m_entityFilters;

	using ComponentTypeSet = std::set<Component::ComponentType>;

	// All entities
	std::shared_ptr<EntityRepository> m_entityRepository;

	// A cache of entities that have no parents
	// TODO: Turn this into a filter?
	std::vector<std::shared_ptr<Entity>> m_rootEntities;

	bool m_initialized = false;

public:
	
	SceneGraphManager(Scene& scene, const std::shared_ptr<EntityRepository> &entityRepository);
	SceneGraphManager(const SceneGraphManager& other) = delete;
	~SceneGraphManager();

	void Initialize() override;
	void Tick() override;
	void Shutdown() override {}

	std::shared_ptr<Entity> Instantiate(const std::string &name);
	std::shared_ptr<Entity> Instantiate(const std::string &name, Entity& parentEntity);

	/**
	 * Will do nothing if no entity exists with the given ID.
	 */
	void Destroy(Entity::ID_TYPE entity);

	std::shared_ptr<Entity> GetEntityPtrById(Entity::ID_TYPE id) const;
	std::shared_ptr<EntityFilter> GetEntityFilter(const ComponentTypeSet &componentTypes, EntityFilterMode mode = EntityFilterMode::ALL);

	void HandleEntityAdd(const Entity &entity);
	void HandleEntityRemove(const Entity &entity);
	void HandleEntityComponentAdd(const Entity &entity, const Component &componentType);
	void HandleEntityComponentRemove(const Entity &entity, const Component &componentType);
	void HandleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>> &newEntities);

private:

	// Entity Lifecycle
	std::shared_ptr<Entity> Instantiate(const std::string &name, Entity *parentEntity);
	void InitializeEntity(const std::shared_ptr<Entity> &entityPtr) const;
	void AddEntityToScene(const std::shared_ptr<Entity> &entityPtr) const;

	std::shared_ptr<Entity> RemoveFromRoot(std::shared_ptr<Entity> entity);
	void AddToRoot(std::shared_ptr<Entity> entity);

	class EntityFilterImpl : public EntityFilter {
	public:
		std::set<Component::ComponentType> m_componentTypes;
		EntityFilterMode m_mode;

		EntityFilterImpl(const ComponentTypeSet::const_iterator &begin, const ComponentTypeSet::const_iterator &end, EntityFilterMode mode);

		void HandleEntityAdd(const std::shared_ptr<Entity> &entity);
		void HandleEntityRemove(const std::shared_ptr<Entity> &entity);
		void HandleEntityComponentsUpdated(const std::shared_ptr<Entity> &entity);

	};
};

}