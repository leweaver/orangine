#pragma once

#include "../Engine/Scene.h"
#include "Mock_scene_graph_manager.h"
#include "Mock_entity_render_manager.h"
#include "Mock_entity_scripting_manager.h"
#include "Mock_asset_manager.h"
#include "Mock_input_manager.h"


#include <functional>

namespace oe_test {

	class Mock_scene : public oe::Scene {
	public:

		explicit Mock_scene()
			: Scene(createManagers())
		{}

	private:
		Manager_tuple createManagers()
		{
			using namespace std;
			using namespace oe;

			Manager_tuple managers;

			/*
			// Repositories
			get<shared_ptr<Entity_repository>>(managers) = make_shared<Entity_repository>(*this);
			get<shared_ptr<Material_repository>>(managers) = make_shared<Material_repository>();

			// Factories
			get<shared_ptr<Primitive_mesh_data_factory>>(managers) = make_shared<Primitive_mesh_data_factory>();
			*/

			// Services / Managers
			get<shared_ptr<IScene_graph_manager>>(managers) = _mockSceneGraphManager = std::make_shared<Mock_scene_graph_manager>(*this);
			get<shared_ptr<IEntity_render_manager>>(managers) = _mockEntityRenderManager = std::make_shared<Mock_entity_render_manager>(*this);
			get<shared_ptr<IEntity_scripting_manager>>(managers) = _mockEntityScriptingManager = std::make_shared<Mock_entity_scripting_manager>(*this);
			get<shared_ptr<IAsset_manager>>(managers) = _mockAssetManager = std::make_shared<Mock_asset_manager>(*this);
			get<shared_ptr<IInput_manager>>(managers) = _mockInputManager = std::make_shared<Mock_input_manager>(*this);

			return managers;
		}

		std::shared_ptr<Mock_scene_graph_manager> _mockSceneGraphManager;
		std::shared_ptr<Mock_entity_render_manager> _mockEntityRenderManager;
		std::shared_ptr<Mock_entity_scripting_manager> _mockEntityScriptingManager;
		std::shared_ptr<Mock_asset_manager> _mockAssetManager;
		std::shared_ptr<Mock_input_manager> _mockInputManager;
	};

}