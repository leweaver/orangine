#include "pch.h"

#include "Mock_scene.h"

#include <string>

using namespace std::string_literals;
using namespace oe;
using namespace oe_test;

class Scene_graph_manager_tests : public ::testing::Test {
protected:
	void SetUp() override
	{
		const auto hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
		ASSERT_TRUE(SUCCEEDED(hr));
	}

	void TearDown() override
	{
		CoUninitialize();
	}

	std::unique_ptr<Mock_scene> createInitMockScene() const
	{
		auto mockScene = std::make_unique<Mock_scene>();
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

		mockScene->initialize();

		return mockScene;
	}
	
	const std::string PARENT_ENTITY_NAME = "PARENT";
	const Entity::Id_type PARENT_ENTITY_ID = 1;

	const std::string CHILD_ENTITY_1_NAME = "CHILD1";
	const Entity::Id_type CHILD_ENTITY_1_ID = 101;
	const std::string CHILD_ENTITY_2_NAME = "CHILD2";
	const Entity::Id_type CHILD_ENTITY_2_ID = 102;
};


TEST_F(Scene_graph_manager_tests, instantiateEntity) 
{
	auto mockScene = std::make_unique<Mock_scene>();
	mockScene->initialize();
	auto sceneGraphManager = std::make_shared<Scene_graph_manager>(*mockScene, mockScene->mockEntityRepository);
	
	{
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(CHILD_ENTITY_1_NAME))))
			.WillOnce(testing::Return(std::make_shared<Entity>(*mockScene, std::string(CHILD_ENTITY_1_NAME), CHILD_ENTITY_1_ID)));

		const auto entity = sceneGraphManager->instantiate(CHILD_ENTITY_1_NAME);

		ASSERT_NE(entity.get(), nullptr);
		EXPECT_EQ(entity->getId(), CHILD_ENTITY_1_ID);
		EXPECT_EQ(entity->getName(), CHILD_ENTITY_1_NAME);
	}

	sceneGraphManager.reset();
	mockScene->shutdown();
}

TEST_F(Scene_graph_manager_tests, entityTransformFromParent)
{
	using namespace DirectX::SimpleMath;

	auto mockScene = createInitMockScene();
	auto sceneGraphManager = std::make_shared<Scene_graph_manager>(*mockScene, mockScene->mockEntityRepository);

	{
		const auto parentEntity = std::make_shared<Entity>(*mockScene, std::string(PARENT_ENTITY_NAME), PARENT_ENTITY_ID);
		const auto child1Entity = std::make_shared<Entity>(*mockScene, std::string(CHILD_ENTITY_1_NAME), CHILD_ENTITY_1_ID);
		const auto child2Entity = std::make_shared<Entity>(*mockScene, std::string(CHILD_ENTITY_1_NAME), CHILD_ENTITY_2_ID);
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(PARENT_ENTITY_NAME))))
			.WillOnce(testing::Return(parentEntity));
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(CHILD_ENTITY_1_NAME))))
			.WillOnce(testing::Return(child1Entity));
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(CHILD_ENTITY_2_NAME))))
			.WillOnce(testing::Return(child2Entity));
		EXPECT_CALL(*mockScene->mockEntityRepository, getEntityPtrById(testing::Eq(PARENT_ENTITY_ID)))
			.WillOnce(testing::Return(parentEntity));
		EXPECT_CALL(*mockScene->mockEntityRepository, getEntityPtrById(testing::Eq(CHILD_ENTITY_1_ID)))
			.WillOnce(testing::Return(child1Entity));

		// These are called from within the Entity
		EXPECT_CALL(*mockScene->mockSceneGraphManager, getEntityPtrById(testing::Eq(PARENT_ENTITY_ID)))
			.WillOnce(testing::Return(parentEntity));
		EXPECT_CALL(*mockScene->mockSceneGraphManager, getEntityPtrById(testing::Eq(CHILD_ENTITY_1_ID)))
			.Times(2)
			.WillRepeatedly(testing::Return(child1Entity));
		EXPECT_CALL(*mockScene->mockSceneGraphManager, getEntityPtrById(testing::Eq(CHILD_ENTITY_2_ID)))
			.WillOnce(testing::Return(child2Entity));
	}

	{
		const auto parentEntity = sceneGraphManager->instantiate(PARENT_ENTITY_NAME);
		ASSERT_NE(parentEntity.get(), nullptr);
		const auto child1Entity = sceneGraphManager->instantiate(CHILD_ENTITY_1_NAME, *parentEntity);
		ASSERT_NE(child1Entity.get(), nullptr);
		ASSERT_TRUE(child1Entity->hasParent());
		const auto child2Entity = sceneGraphManager->instantiate(CHILD_ENTITY_2_NAME, *child1Entity);
		ASSERT_NE(child2Entity.get(), nullptr);
		ASSERT_TRUE(child2Entity->hasParent());

		// Test translation
		parentEntity->setPosition({ 1.0f, 2.0f, 3.0f });
		child1Entity->setPosition({ 1.0f, 2.0f, 3.0f });
		child2Entity->setPosition({ 1.0f, 2.0f, 3.0f });
		sceneGraphManager->tick();
		EXPECT_FLOAT_EQ(child2Entity->worldPosition().x, 3.0f);
		EXPECT_FLOAT_EQ(child2Entity->worldPosition().y, 6.0f);
		EXPECT_FLOAT_EQ(child2Entity->worldPosition().z, 9.0f);

		// Test rotation around the Y axis towards 'forward' (-Z)
		parentEntity->setPosition(Vector3::Zero);
		child1Entity->setPosition(Vector3::UnitX);
		child2Entity->setPosition(Vector3::UnitX);
		parentEntity->setRotation(Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XM_PIDIV2));
		child1Entity->setRotation(Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XM_PIDIV2));

		sceneGraphManager->tick();

		EXPECT_NEAR(child1Entity->worldPosition().x, 0.0f, 0.000001f);
		EXPECT_NEAR(child1Entity->worldPosition().y, 0.0f, 0.000001f);
		EXPECT_NEAR(child1Entity->worldPosition().z, -1.0f, 0.000001f);

		EXPECT_NEAR(child2Entity->worldPosition().x, -1.0f, 0.000001f);
		EXPECT_NEAR(child2Entity->worldPosition().y, 0.0f, 0.000001f);
		EXPECT_NEAR(child2Entity->worldPosition().z, -1.0f, 0.000001f);
	}

	sceneGraphManager.reset();
	mockScene->shutdown();
}