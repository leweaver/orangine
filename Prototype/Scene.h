#pragma once

#include "EntityManager.h"
#include "EntityRenderer.h"

namespace OE {
	class Scene
	{
		std::unique_ptr<EntityManager> m_entityManager;
		std::unique_ptr<EntityRenderer> m_entityRenderer;
	public:
		Scene();
		~Scene();

		EntityManager& EntityManager() const { return *m_entityManager; }
		EntityRenderer& EntityRenderer() const { return *m_entityRenderer; }

		/*
		 * Functions that can be used to send events to various managers
		 */
		void OnComponentAdded(Entity& entity, Component& component) const;
		void OnComponentRemoved(Entity& entity, Component& component) const;
		void OnEntityAdded(Entity& entity) const;
		void OnEntityRemoved(Entity& entity) const;
	};

}
