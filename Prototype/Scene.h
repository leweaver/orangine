#pragma once

#include "StepTimer.h"
#include "Entity_graph_loader.h"

#include "Entity_render_manager.h"
#include "Entity_repository.h"
#include "Scene_graph_manager.h"
#include "Entity_scripting_manager.h"
#include "Asset_manager.h"

#include <map>

namespace oe {
	class Entity;
	class Component;

	class Scene
	{
		std::shared_ptr<Entity_repository> _entityRepository;
		std::shared_ptr<Material_repository> _materialRepository;

		std::shared_ptr<Primitive_mesh_data_factory> _primitiveMeshDataFactory;

		std::unique_ptr<Scene_graph_manager> _sceneGraphManager;
		std::unique_ptr<Entity_render_manager> _entityRenderManager;
		std::unique_ptr<Entity_scripting_manager> _entityScriptinigManager;
		std::unique_ptr<Asset_manager> _assetManager;
		
		std::map<std::string, std::shared_ptr<Entity_graph_loader>> _entityGraphLoaders;

		std::shared_ptr<Entity> _mainCamera;

		// Values as of the last call to Tick.
		double _deltaTime = 0;
		double _elapsedTime = 0;

	public:
		Scene(DX::DeviceResources& deviceResources);
		~Scene() = default;

		Scene_graph_manager& sceneGraphManager() const { return *_sceneGraphManager; }
		Entity_render_manager& entityRenderManger() const { return *_entityRenderManager; }
		Entity_scripting_manager& entityScriptingManager() const { return *_entityScriptinigManager; }
		Asset_manager& assetManager() const { return *_assetManager; }

		void tick(DX::StepTimer const& timer);
		void shutdown();

		/** Total time since game start, in seconds. */
		double elapsedTime() const { return _elapsedTime; }

		/** Time since last frame, in seconds. */
		double deltaTime() const { return _deltaTime; }

		/*
		 * Functions that can be used to send events to various managers
		 */
		void onComponentAdded(Entity& entity, Component& component) const;
		void onComponentRemoved(Entity& entity, Component& component) const;
		void onEntityAdded(Entity& entity) const;
		void onEntityRemoved(Entity& entity) const;

		/*
		 * Add things to the scene, from a file
		 */
		void loadEntities(const std::string& filename);
		void loadEntities(const std::string& filename, Entity& parentEntity);

		template <typename TLoader>
		std::shared_ptr<TLoader> AddMeshLoader()
		{
			const std::shared_ptr<TLoader> ml = std::make_shared<TLoader>();
			std::vector<std::string> extensions;
			ml->getSupportedFileExtensions(extensions);
			for (const auto& extension : extensions) {
				if (_entityGraphLoaders.find(extension) != _entityGraphLoaders.end())
					throw std::runtime_error("Failed to register entity graph loader, file extension is already registered: " + extension);
				_entityGraphLoaders[extension] = ml;
			}
			return ml;
		}

		std::shared_ptr<Entity> mainCamera() const { return _mainCamera; };
		void setMainCamera(const std::shared_ptr<Entity>& camera);

	private:
		void loadEntities(const std::string& filename, Entity* parentEntity);
	};

}
