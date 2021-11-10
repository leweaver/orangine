#pragma once

#include <OeCore/IEntity_repository.h>

namespace oe {
class Entity_repository : public IEntity_repository {
  Entity::Entity_ptr_map _entities;
  int _lastEntityId;

 public:
  Entity_repository();

  // IEntity_repository implementation
  std::shared_ptr<Entity>
  instantiate(std::string_view name, IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory) override;
  void remove(Entity::Id_type id) override;
  std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) override;
  Entity& getEntityById(Entity::Id_type id) override;
};
}// namespace oe