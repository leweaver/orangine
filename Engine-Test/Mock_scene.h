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

		static std::unique_ptr<Mock_scene> createInit(bool expectCalls = true)
		{
			auto mockScene = std::make_unique<Mock_scene>();

			if (expectCalls) {
				EXPECT_CALL(*mockScene->mockAssetManager, initialize());
				EXPECT_CALL(*mockScene->mockAssetManager, shutdown());
				EXPECT_CALL(*mockScene->mockEntityRenderManager, initialize());
				EXPECT_CALL(*mockScene->mockEntityRenderManager, shutdown());
				EXPECT_CALL(*mockScene->mockEntityScriptingManager, initialize());
				EXPECT_CALL(*mockScene->mockEntityScriptingManager, shutdown());
				EXPECT_CALL(*mockScene->mockInputManager, initialize());
				EXPECT_CALL(*mockScene->mockInputManager, shutdown());
				EXPECT_CALL(*mockScene->mockSceneGraphManager, initialize());
				EXPECT_CALL(*mockScene->mockSceneGraphManager, shutdown());
			}

			mockScene->initialize();

			return mockScene;
		}

		Mock_scene()
		{
			mockSceneGraphManager = std::make_shared<Mock_scene_graph_manager>(*this);
			mockEntityRenderManager = std::make_shared<Mock_entity_render_manager>(*this);
			mockEntityScriptingManager = std::make_shared<Mock_entity_scripting_manager>(*this);
			mockAssetManager = std::make_shared<Mock_asset_manager>(*this);
			mockInputManager = std::make_shared<Mock_input_manager>(*this);
			mockEntityRepository = std::make_shared<Mock_entity_repository>();
		}

		void initialize() override
		{
			using namespace std;
			using namespace oe;
			
			// Repositories
			get<shared_ptr<IEntity_repository>>(_managers) = mockEntityRepository;
			/*
			get<shared_ptr<Material_repository>>(managers) = make_shared<Material_repository>();

			// Factories
			get<shared_ptr<Primitive_mesh_data_factory>>(managers) = make_shared<Primitive_mesh_data_factory>();
			*/

			// Services / Managers
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
		}

		std::shared_ptr<Mock_entity_repository> mockEntityRepository;

		std::shared_ptr<Mock_scene_graph_manager> mockSceneGraphManager;
		std::shared_ptr<Mock_entity_render_manager> mockEntityRenderManager;
		std::shared_ptr<Mock_entity_scripting_manager> mockEntityScriptingManager;
		std::shared_ptr<Mock_asset_manager> mockAssetManager;
		std::shared_ptr<Mock_input_manager> mockInputManager;
	};

}