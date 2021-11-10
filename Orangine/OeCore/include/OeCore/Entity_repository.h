#pragma once

#include "OeCore/Entity.h"

#include <memory>

namespace oe {
	class Scene;
  class IComponent_factory;

	class IEntity_repository {
	public:
		virtual std::shared_ptr<Entity> instantiate(std::string_view name, IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory) = 0;

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

	class Entity_repository : public IEntity_repository
	{
		Entity::Entity_ptr_map _entities;
		int _lastEntityId;

	public:
		Entity_repository();

		// IEntity_repository implementation
		std::shared_ptr<Entity> instantiate(std::string_view name, IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory) override;
		void remove(Entity::Id_type id) override;
		std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) override;
		Entity& getEntityById(Entity::Id_type id) override;
	};
}
