#pragma once

#include "Entity.h"

#include <memory>

namespace oe {
class IComponent_factory;
class IScene_graph_manager;

class IEntity_repository {
 public:
  virtual std::shared_ptr<Entity>
  instantiate(std::string_view name, IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory) = 0;

  /**
		* Will do nothing if an entity with given ID doesn't exist.
		*/
  virtual void remove(Entity::Id_type id) = 0;

  /**
		 * Will return nullptr if no entity exists
		 */
  virtual std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) = 0;

  /**
		 * Will assert that entity with given ID exists.
		 */
  virtual Entity& getEntityById(Entity::Id_type id) = 0;
};
}