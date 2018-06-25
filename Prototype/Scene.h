#pragma once

#include "StepTimer.h"
#include "Entity_graph_loader.h"

#include "Entity_render_manager.h"
#include "Entity_repository.h"
#include "Scene_graph_manager.h"
#include "Entity_scripting_manager.h"
#include "Asset_manager.h"

#include <map>
#include "Input_manager.h"
#include <memory>

namespace oe {
	class Entity;
	class Component;

	class Scene
	{
	public:
		explicit Scene(DX::DeviceResources& deviceResources);
		~Scene() = default;

		void tick(DX::StepTimer const& timer);
		void shutdown();

		template <typename TMgr>
		TMgr& manager() const;

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
		std::shared_ptr<TLoader> addMeshLoader()
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
		
		std::tuple<
			// Repositories
			std::shared_ptr<Entity_repository>,
			std::shared_ptr<Material_repository>,

			// Factories
			std::shared_ptr<Primitive_mesh_data_factory>,

			// Managers
			std::unique_ptr<Scene_graph_manager>,
			std::unique_ptr<Entity_render_manager>,
			std::unique_ptr<Entity_scripting_manager>,
			std::unique_ptr<Asset_manager>,
			std::unique_ptr<Input_manager>
		> _managers;

		std::map<std::string, std::shared_ptr<Entity_graph_loader>> _entityGraphLoaders;

		std::shared_ptr<Entity> _mainCamera;

		// Values as of the last call to Tick.
		double _deltaTime = 0;
		double _elapsedTime = 0;

		// Templated helper methods		
		Scene_graph_manager& sceneGraphManager() const { return *std::get<std::unique_ptr<Scene_graph_manager>>(_managers); }
		Entity_render_manager& entityRenderManger() const { return *std::get<std::unique_ptr<Entity_render_manager>>(_managers); }
		Entity_scripting_manager& entityScriptingManager() const { return *std::get<std::unique_ptr<Entity_scripting_manager>>(_managers); }
		Asset_manager& assetManager() const { return *std::get<std::unique_ptr<Asset_manager>>(_managers); }
		Input_manager& inputManager() const { return *std::get<std::unique_ptr<Input_manager>>(_managers); }

		template<int TIdx = 0>
		constexpr void initializeManagers()
		{
			// Initialize only types that derive from Manager_base
			if constexpr (std::is_base_of_v<Manager_base, std::remove_pointer_t<decltype(std::get<TIdx>(_managers).get())>>)
				std::get<TIdx>(_managers)->initialize();				

			// iterate the next manager
			if constexpr (TIdx + 1 < std::tuple_size_v<decltype(_managers)>)
				initializeManagers<TIdx + 1>();
		}

		template<int TIdx = 0>
		constexpr void tickManagers()
		{
			using namespace std; 
			
			// Tick only types that derive from Manager_base
			if constexpr (is_base_of_v<Manager_tickable, remove_pointer_t<decltype(get<TIdx>(_managers).get())>>)
				get<TIdx>(_managers)->tick();

			// Recursively iterate to the next tuple index
			if constexpr (TIdx + 1 < tuple_size_v<decltype(_managers)>)
				tickManagers<TIdx + 1>();
		}

		template<int TIdx = std::tuple_size_v<decltype(_managers)> - 1>
		constexpr void shutdownManagers()
		{
			// Initialize only types that derive from Manager_base
			if constexpr (std::is_base_of_v<Manager_base, std::remove_pointer_t<decltype(std::get<TIdx>(_managers).get())>>)
				std::get<TIdx>(_managers)->shutdown();

			std::get<TIdx>(_managers).reset();

			// iterate the next manager (in reverse order than initialized)
			if constexpr (TIdx > 0)
				shutdownManagers<TIdx - 1>();
		}
	};

	template <typename TMgr>
	TMgr& Scene::manager() const
	{
		return *std::get<std::unique_ptr<TMgr>>(_managers);
	}

}
