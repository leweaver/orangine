#pragma once

#include "Entity_filter_impl.h"

#include <OeCore/Component.h>
#include <OeCore/Entity.h>
#include <OeCore/Entity_repository.h>
#include <OeCore/IScene_graph_manager.h>

#include <vector>

namespace oe::internal {

class Scene_graph_manager : public IScene_graph_manager {
  friend Entity;
  friend EntityRef;

 public:
  Scene_graph_manager(Scene& scene, std::shared_ptr<IEntity_repository> entityRepository);
  Scene_graph_manager(const Scene_graph_manager& other) = delete;
  ~Scene_graph_manager() = default;

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_tickable implementation
  void tick() override;

  // Creates a deep clone of the given entity. Useful for instantiating prefabs
  std::shared_ptr<Entity> clone(const Entity& entity, Entity* newParent = nullptr);

  std::shared_ptr<Entity> instantiate(const std::string& name) override;
  std::shared_ptr<Entity> instantiate(const std::string& name, Entity& parentEntity) override;

  void renderImGui() override;

  /**
   * Will do nothing if no entity exists with the given ID.
   */
  void destroy(Entity::Id_type entityId) override;

  std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) const override;
  std::shared_ptr<Entity_filter> getEntityFilter(
      const Component_type_set& componentTypes,
      Entity_filter_mode mode = Entity_filter_mode::All) override;

  void handleEntityAdd(const Entity& entity) override;
  void handleEntityRemove(const Entity& entity) override;
  void handleEntityComponentAdd(const Entity& entity, const Component& componentType) override;
  void handleEntityComponentRemove(const Entity& entity, const Component& componentType) override;
  void handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>>& loadedEntities) override;
  bool findCollidingEntity(
      const Ray& ray,
      Entity const*& foundEntity,
      Ray_intersection& foundIntersection,
      float maxDistance = FLT_MAX) override;

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
  static void updateEntity(Entity* entity);

  static std::string _name;

  std::vector<std::shared_ptr<Entity_filter_impl>> m_entityFilters;

  // All entities
  std::shared_ptr<IEntity_repository> _entityRepository;

  // A cache of entities that have no parents
  // TODO: Turn this into a filter?
  std::vector<std::shared_ptr<Entity>> _rootEntities;

  bool _initialized = false;
};

} // namespace oe::internal
