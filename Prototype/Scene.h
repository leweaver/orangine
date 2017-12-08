#pragma once

#include "StepTimer.h"

namespace OE {
	class SceneGraphManager;
	class EntityRenderManager;
	class EntityScriptingManager;
	class Entity;
	class Component;

	class Scene
	{
		std::unique_ptr<SceneGraphManager> m_entityManager;
		std::unique_ptr<EntityRenderManager> m_entityRenderer;
		std::unique_ptr<EntityScriptingManager> m_entityScriptinigManager;

		// Values as of the last call to Tick.
		double m_deltaTime = 0;
		double m_elapsedTime = 0;

	public:
		Scene();
		~Scene();

		SceneGraphManager& GetEntityManager() const { return *m_entityManager; }
		EntityRenderManager& GetEntityRenderService() const { return *m_entityRenderer; }
		EntityScriptingManager& GetEntityScriptingManager() const { return *m_entityScriptinigManager; }

		void Tick(DX::StepTimer const& timer);

		/** Total time since game start, in seconds. */
		const double& GetElapsedTime() const { return m_elapsedTime; }

		/** Time since last frame, in seconds. */
		const double& GetDeltaTime() const { return m_deltaTime; }

		/*
		 * Functions that can be used to send events to various managers
		 */
		void OnComponentAdded(Entity& entity, Component& component) const;
		void OnComponentRemoved(Entity& entity, Component& component) const;
		void OnEntityAdded(Entity& entity) const;
		void OnEntityRemoved(Entity& entity) const;
	};

}
