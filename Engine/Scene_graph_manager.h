#pragma once
#include "Entity.h"
#include "Component.h"
#include "Entity_filter.h"
#include "IScene_graph_manager.h"
#include "Entity_repository.h"

#include <vector>
#include <set>

namespace oe::internal {

class Scene_graph_manager : public IScene_graph_manager {
	friend Entity;
	friend EntityRef;
	class Entity_filter_impl;
public:
	
	Scene_graph_manager(Scene& scene, std::shared_ptr<IEntity_repository> entityRepository);
	Scene_graph_manager(const Scene_graph_manager& other) = delete;
	~Scene_graph_manager() = default;
	
	// Manager_base implementation
	void initialize() override;
	void shutdown() override;

	// Manager_tickable implementation
	void tick() override;

	std::shared_ptr<Entity> instantiate(const std::string& name) override;
	std::shared_ptr<Entity> instantiate(const std::string& name, Entity& parentEntity) override;

    void renderImGui() override;

	/**
	 * Will do nothing if no entity exists with the given ID.
	 */
	void destroy(Entity::Id_type entityId) override;

	std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) const override;
	std::shared_ptr<Entity_filter> getEntityFilter(const Component_type_set& componentTypes, Entity_filter_mode mode = Entity_filter_mode::All) override;
	
	void handleEntityAdd(const Entity& entity) override;
	void handleEntityRemove(const Entity& entity) override;
	void handleEntityComponentAdd(const Entity& entity, const Component& componentType) override;
	void handleEntityComponentRemove(const Entity& entity, const Component& componentType) override;
	void handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>>& loadedEntities) override;

private:

	std::shared_ptr<Entity> removeFromRoot(std::shared_ptr<Entity> entity) override;
	void addToRoot(std::shared_ptr<Entity> entity) override;

	// Entity Lifecycle
	std::shared_ptr<Entity> instantiate(const std::string& name, Entity* parentEntity);
	void initializeEntity(std::shared_ptr<Entity> entityPtr) const;
	void addEntityToScene(std::shared_ptr<Entity> entityPtr) const;

	/**
	* Applies transforms recursively down (from root -> leaves),
	* then updates components from bottom up (from leaves -> root)
	*/
	void updateEntity(Entity* entity);

	class Entity_filter_impl : public Entity_filter {
	public:
		std::set<Component::Component_type> componentTypes;
		Entity_filter_mode mode;

		Entity_filter_impl(const Component_type_set::const_iterator& begin, const Component_type_set::const_iterator& end, Entity_filter_mode mode);

		void handleEntityAdd(const std::shared_ptr<Entity>& entity);
		void handleEntityRemove(const std::shared_ptr<Entity>& entity);
		void handleEntityComponentsUpdated(const std::shared_ptr<Entity>& entity);

	};

	std::vector<std::shared_ptr<Entity_filter_impl>> m_entityFilters;
	
	// All entities
	std::shared_ptr<IEntity_repository> _entityRepository;

	// A cache of entities that have no parents
	// TODO: Turn this into a filter?
	std::vector<std::shared_ptr<Entity>> _rootEntities;

	bool _initialized = false;
};

}
