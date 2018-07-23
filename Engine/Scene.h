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
		void setMainCamera(const std::shared_ptr<Entity>& cameraEntity);

	protected:
		using Manager_tuple = std::tuple<
			// Repositories
			std::shared_ptr<Entity_repository>,
			std::shared_ptr<Material_repository>,

			// Factories
			std::shared_ptr<Primitive_mesh_data_factory>,

			// Managers
			std::unique_ptr<IScene_graph_manager>,
			std::unique_ptr<Entity_render_manager>,
			std::unique_ptr<Entity_scripting_manager>,
			std::unique_ptr<Asset_manager>,
			std::unique_ptr<Input_manager>
		>;

		Scene(Manager_tuple&& managers);

		Manager_tuple _managers;

	private:

		void loadEntities(const std::string& filename, Entity* parentEntity);

		std::map<std::string, std::shared_ptr<Entity_graph_loader>> _entityGraphLoaders;

		std::shared_ptr<Entity> _mainCamera;

		// Values as of the last call to Tick.
		double _deltaTime = 0;
		double _elapsedTime = 0;

		// Templated helper methods		
		IScene_graph_manager& sceneGraphManager() const { return *std::get<std::unique_ptr<IScene_graph_manager>>(_managers); }
		Entity_render_manager& entityRenderManger() const { return *std::get<std::unique_ptr<Entity_render_manager>>(_managers); }
		Entity_scripting_manager& entityScriptingManager() const { return *std::get<std::unique_ptr<Entity_scripting_manager>>(_managers); }
		Asset_manager& assetManager() const { return *std::get<std::unique_ptr<Asset_manager>>(_managers); }
		Input_manager& inputManager() const { return *std::get<std::unique_ptr<Input_manager>>(_managers); }
	};

	template <typename TMgr>
	TMgr& Scene::manager() const
	{
		return *std::get<std::unique_ptr<TMgr>>(_managers);
	}

	class Scene_device_resource_aware : public Scene {
	public:

		explicit Scene_device_resource_aware(DX::DeviceResources& deviceResources);

		void createWindowSizeDependentResources(HWND window, int width, int height);
		void destroyWindowSizeDependentResources();
		void createDeviceDependentResources();
		void destroyDeviceDependentResources();
		void processMessage(UINT message, WPARAM wParam, LPARAM lParam) const;

		Manager_tuple createManagers(DX::DeviceResources& deviceResources);

	private:
		DX::DeviceResources& _deviceResources;
	};
}
