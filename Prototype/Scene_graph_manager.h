#pragma once
#include "Entity.h"
#include "Component.h"
#include "Entity_filter.h"
#include "Manager_base.h"
#include "Entity_repository.h"

#include <vector>
#include <set>

namespace oe {
	
enum class Entity_filter_mode
{
	All,
	Any
};


class Scene_graph_manager : public Manager_base
{
	friend Entity;
	friend EntityRef;
	class Entity_filter_impl;

	std::vector<std::shared_ptr<Entity_filter_impl>> m_entityFilters;

	using Component_type_set = std::set<Component::Component_type>;

	// All entities
	std::shared_ptr<Entity_repository> _entityRepository;

	// A cache of entities that have no parents
	// TODO: Turn this into a filter?
	std::vector<std::shared_ptr<Entity>> _rootEntities;

	bool _initialized = false;

public:
	
	Scene_graph_manager(Scene& scene, std::shared_ptr<Entity_repository> entityRepository);
	Scene_graph_manager(const Scene_graph_manager& other) = delete;
	~Scene_graph_manager();

	void initialize() override;
	void tick() override;
	void shutdown() override {}

	std::shared_ptr<Entity> instantiate(const std::string& name);
	std::shared_ptr<Entity> instantiate(const std::string& name, Entity& parentEntity);

	/**
	 * Will do nothing if no entity exists with the given ID.
	 */
	void destroy(Entity::Id_type entity);

	std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) const;
	std::shared_ptr<Entity_filter> getEntityFilter(const Component_type_set& componentTypes, Entity_filter_mode mode = Entity_filter_mode::All);

	void handleEntityAdd(const Entity& entity);
	void handleEntityRemove(const Entity& entity);
	void handleEntityComponentAdd(const Entity& entity, const Component& componentType);
	void handleEntityComponentRemove(const Entity& entity, const Component& componentType);
	void handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>>& loadedEntities);

private:

	// Entity Lifecycle
	std::shared_ptr<Entity> instantiate(const std::string& name, Entity* parentEntity);
	void initializeEntity(std::shared_ptr<Entity> entityPtr) const;
	void addEntityToScene(std::shared_ptr<Entity> entityPtr) const;

	std::shared_ptr<Entity> removeFromRoot(std::shared_ptr<Entity> entity);
	void addToRoot(std::shared_ptr<Entity> entity);

	class Entity_filter_impl : public Entity_filter {
	public:
		std::set<Component::Component_type> componentTypes;
		Entity_filter_mode mode;

		Entity_filter_impl(const Component_type_set::const_iterator& begin, const Component_type_set::const_iterator& end, Entity_filter_mode mode);

		void handleEntityAdd(const std::shared_ptr<Entity>& entity);
		void handleEntityRemove(const std::shared_ptr<Entity>& entity);
		void handleEntityComponentsUpdated(const std::shared_ptr<Entity>& entity);

	};
};

}
