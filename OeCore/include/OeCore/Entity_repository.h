#pragma once

#include "OeCore/Entity.h"

#include <memory>

namespace oe {
	class Scene;

	class IEntity_repository {
	public:
		virtual std::shared_ptr<Entity> instantiate(std::string_view name) = 0;

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
		Scene& _scene;

	public:
		explicit Entity_repository(Scene& scene);

		// IEntity_repository implementation
		std::shared_ptr<Entity> instantiate(std::string_view name) override;
		void remove(Entity::Id_type id) override;
		std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) override;
		Entity& getEntityById(Entity::Id_type id) override;
	};
}
