#pragma once

#include "StepTimer.h"
#include "EntityGraphLoader.h"
#include <map>

namespace OE {
	class SceneGraphManager;
	class EntityRenderManager;
	class EntityScriptingManager;
	class AssetManager;
	class Entity;
	class Component;

	class Scene
	{
		std::shared_ptr<EntityRepository> m_entityRepository;
		std::shared_ptr<MaterialRepository> m_materialRepository;

		std::unique_ptr<SceneGraphManager> m_sceneGraphManager;
		std::unique_ptr<EntityRenderManager> m_entityRenderManager;
		std::unique_ptr<EntityScriptingManager> m_entityScriptinigManager;
		std::unique_ptr<AssetManager> m_assetManager;

		std::map<std::string, std::shared_ptr<EntityGraphLoader>> m_entityGraphLoaders;

		// Values as of the last call to Tick.
		double m_deltaTime = 0;
		double m_elapsedTime = 0;

	public:
		Scene(DX::DeviceResources &deviceResources);
		~Scene();

		SceneGraphManager &GetSceneGraphManager() const { return *m_sceneGraphManager; }
		EntityRenderManager &GetEntityRenderManger() const { return *m_entityRenderManager; }
		EntityScriptingManager &GetEntityScriptingManager() const { return *m_entityScriptinigManager; }
		AssetManager &GetAssetManager() const { return *m_assetManager; }

		void Tick(DX::StepTimer const& timer);
		void Shutdown();

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

		/*
		 * Add things to the scene, from a file
		 */
		void LoadEntities(const std::string &filename);
		void LoadEntities(const std::string &filename, Entity &parentEntity);

		template <typename TLoader>
		std::shared_ptr<TLoader> AddMeshLoader()
		{
			const std::shared_ptr<TLoader> ml = std::make_shared<TLoader>();
			std::vector<std::string> extensions;
			ml->GetSupportedFileExtensions(extensions);
			for (const auto extension : extensions) {
				if (m_entityGraphLoaders.find(extension) != m_entityGraphLoaders.end())
					throw std::runtime_error("Failed to register entity graph loader, file extension is already registered: " + extension);
				m_entityGraphLoaders[extension] = ml;
			}
			return ml;
		}

	private:
		void LoadEntities(const std::string& filename, Entity *parentEntity);
	};

}
