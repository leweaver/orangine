#pragma once
#include "Entity.h"
#include "Component.h"
#include "EntityFilter.h"
#include "ManagerBase.h"
#include "EntityRepository.h"

#include <vector>
#include <set>

namespace OE {

class SceneGraphManager : public ManagerBase
{
	friend Entity;
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

	Entity& Instantiate(const std::string &name);
	Entity& Instantiate(const std::string &name, Entity& parentEntity);

	/**
	 * Will do nothing if no entity exists with the given ID.
	 */
	void Destroy(const Entity::ID_TYPE& entity);

	Entity &GetEntityById(const Entity::ID_TYPE id) const;
	std::shared_ptr<EntityFilter> GetEntityFilter(const ComponentTypeSet &componentTypes);

	void HandleEntityAdd(const Entity &entity);
	void HandleEntityRemove(const Entity &entity);
	void HandleEntityComponentAdd(const Entity &entity, const Component &componentType);
	void HandleEntityComponentRemove(const Entity &entity, const Component &componentType);
	void HandleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>> &newEntities);

private:

	// Entity Lifecycle
	Entity& Instantiate(const std::string &name, Entity *parentEntity);
	void InitializeEntity(const std::shared_ptr<Entity> &entityPtr) const;
	void AddEntityToScene(const std::shared_ptr<Entity> &entityPtr) const;

	std::shared_ptr<Entity> GetEntityPtrById(const Entity::ID_TYPE id) const;
	std::shared_ptr<Entity> RemoveFromRoot(std::shared_ptr<Entity> entity);
	void AddToRoot(std::shared_ptr<Entity> entity);

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
