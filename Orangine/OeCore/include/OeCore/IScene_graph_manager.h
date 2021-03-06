#pragma once

#include "Component.h"
#include "Dispatcher.h"
#include "Entity.h"
#include "Entity_filter.h"
#include "Entity_filter_mode.h"
#include "Manager_base.h"

#include <OeCore/Entity_graph_loader.h>

#include <unordered_set>
#include <vector>

namespace oe {
struct Ray;
struct Ray_intersection;

class IScene_graph_manager
{
 public:
  using Component_type_set = std::unordered_set<Component::Component_type>;

  virtual ~IScene_graph_manager() = default;

  virtual std::shared_ptr<Entity> instantiate(const std::string& name) = 0;
  virtual std::shared_ptr<Entity> instantiate(const std::string& name, Entity& parentEntity) = 0;
  virtual void renderImGui() = 0;

  virtual void addLoader(std::unique_ptr<Entity_graph_loader> loader) = 0;
  virtual void loadFile(const std::string& filename) = 0;
  virtual void loadFile(const std::string& filename, Entity* parentEntity) = 0;

  /**
   * Will do nothing if no entity exists with the given ID.
   */
  virtual void destroy(Entity::Id_type entityId) = 0;

  virtual std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) const = 0;
  virtual std::shared_ptr<Entity_filter> getEntityFilter(
      const Component_type_set& componentTypes,
      Entity_filter_mode mode = Entity_filter_mode::All) = 0;

  virtual void handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>>& loadedEntities) = 0;

  /**
   * Collision
   */

  // Finds the first entity whose bounding sphere collides with the given ray. This might not be the
  // entity whose center is the closest to the ray origin.
  virtual bool findCollidingEntity(
      const Ray& ray,
      Entity const*& foundEntity,
      Ray_intersection& foundIntersection,
      float maxDistance = FLT_MAX) = 0;

  // Internal use only.
  virtual std::shared_ptr<Entity> removeFromRoot(std::shared_ptr<Entity> entity) = 0;

  // Internal use only.
  virtual void addToRoot(std::shared_ptr<Entity> entity) = 0;

  virtual Dispatcher<Entity&>& getEntityAddedDispatcher() = 0;
};
} // namespace oe
