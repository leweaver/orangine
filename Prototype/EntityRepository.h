#pragma once

#include "Scene.h"
#include "Entity.h"

#include <memory>

namespace OE {

	class EntityRepository
	{
		Entity::EntityPtrMap m_entities;
		int lastEntityId;
		Scene &m_scene;

	public:
		explicit EntityRepository(Scene &scene);
		std::shared_ptr<Entity> Instantiate(const std::string &name);

		/**
		* Will do nothing if an entity with given ID doesn't exist.
		*/
		void Remove(const Entity::ID_TYPE id);

		/**
		 * Will return nullptr if no entity exists
		 */
		std::shared_ptr<Entity> GetEntityPtrById(const Entity::ID_TYPE id);

		/**
		 * Will assert that entity with given ID exists.
		 */
		Entity &GetEntityById(const Entity::ID_TYPE id);
	};
}
