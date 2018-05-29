#pragma once

#include "Scene.h"
#include "Entity.h"

#include <memory>

namespace oe {

	class Entity_repository
	{
		Entity::Entity_ptr_map _entities;
		int _lastEntityId;
		Scene& _scene;

	public:
		explicit Entity_repository(Scene& scene);
		std::shared_ptr<Entity> instantiate(std::string_view name);

		/**
		* Will do nothing if an entity with given ID doesn't exist.
		*/
		void remove(Entity::Id_type id);

		/**
		 * Will return nullptr if no entity exists
		 */
		std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id);

		/**
		 * Will assert that entity with given ID exists.
		 */
		Entity& getEntityById(Entity::Id_type id);
	};
}
