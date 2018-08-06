#pragma once

#include "../Engine/Scene.h"

#include "Mock_entity_repository.h"

#include "Mock_scene_graph_manager.h"
#include "Mock_entity_render_manager.h"
#include "Mock_entity_scripting_manager.h"
#include "Mock_asset_manager.h"
#include "Mock_input_manager.h"


#include <functional>

namespace oe_test {

	class Mock_scene : public oe::Scene {
	public:

		void initialize() override
		{
			using namespace std;
			using namespace oe;
			
			// Repositories
			mockEntityRepository = make_shared<Mock_entity_repository>();
			get<shared_ptr<IEntity_repository>>(_managers) = mockEntityRepository;
			/*
			get<shared_ptr<Material_repository>>(managers) = make_shared<Material_repository>();

			// Factories
			get<shared_ptr<Primitive_mesh_data_factory>>(managers) = make_shared<Primitive_mesh_data_factory>();
			*/

			// Services / Managers
			mockSceneGraphManager = std::make_shared<Mock_scene_graph_manager>(*this);
			mockEntityRenderManager = std::make_shared<Mock_entity_render_manager>(*this);
			mockEntityScriptingManager = std::make_shared<Mock_entity_scripting_manager>(*this);
			mockAssetManager = std::make_shared<Mock_asset_manager>(*this);
			mockInputManager = std::make_shared<Mock_input_manager>(*this);
			get<shared_ptr<IScene_graph_manager>>(_managers) = mockSceneGraphManager;
			get<shared_ptr<IEntity_render_manager>>(_managers) = mockEntityRenderManager;
			get<shared_ptr<IEntity_scripting_manager>>(_managers) = mockEntityScriptingManager;
			get<shared_ptr<IAsset_manager>>(_managers) = mockAssetManager;
			get<shared_ptr<IInput_manager>>(_managers) = mockInputManager;

			Scene::initialize();
		}

		void shutdown() override
		{
			Scene::shutdown();
			
			mockEntityRepository.reset();

			mockSceneGraphManager.reset();
			mockEntityRenderManager.reset();
			mockEntityScriptingManager.reset();
			mockAssetManager.reset();
			mockInputManager.reset();
		}

		std::shared_ptr<Mock_entity_repository> mockEntityRepository;

		std::shared_ptr<Mock_scene_graph_manager> mockSceneGraphManager;
		std::shared_ptr<Mock_entity_render_manager> mockEntityRenderManager;
		std::shared_ptr<Mock_entity_scripting_manager> mockEntityScriptingManager;
		std::shared_ptr<Mock_asset_manager> mockAssetManager;
		std::shared_ptr<Mock_input_manager> mockInputManager;
	};

}