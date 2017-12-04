#pragma once


namespace OE {
	class EntityManager;
	class EntityRenderService;
	class Entity;
	class Component;

	class Scene
	{
		std::unique_ptr<EntityManager> m_entityManager;
		std::unique_ptr<EntityRenderService> m_entityRenderer;
	public:
		Scene();
		~Scene();

		EntityManager& EntityManager() const { return *m_entityManager; }
		EntityRenderService& EntityRenderService() const { return *m_entityRenderer; }

		/*
		 * Functions that can be used to send events to various managers
		 */
		void OnComponentAdded(Entity& entity, Component& component) const;
		void OnComponentRemoved(Entity& entity, Component& component) const;
		void OnEntityAdded(Entity& entity) const;
		void OnEntityRemoved(Entity& entity) const;
	};

}
