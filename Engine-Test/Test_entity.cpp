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
	
	const std::string PARENT_ENTITY_NAME = "PARENT";
	const Entity::Id_type PARENT_ENTITY_ID = 1;

	const std::string CHILD_ENTITY_1_NAME = "CHILD1";
	const Entity::Id_type CHILD_ENTITY_1_ID = 101;
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
	auto mockScene = std::make_unique<Mock_scene>();
	mockScene->initialize();
	auto sceneGraphManager = std::make_shared<Scene_graph_manager>(*mockScene, mockScene->mockEntityRepository);

	{
		const auto parentEntity = std::make_shared<Entity>(*mockScene, std::string(PARENT_ENTITY_NAME), PARENT_ENTITY_ID);
		const auto childEntity = std::make_shared<Entity>(*mockScene, std::string(CHILD_ENTITY_1_NAME), CHILD_ENTITY_1_ID);
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(CHILD_ENTITY_1_NAME))))
			.WillOnce(testing::Return(childEntity));
		EXPECT_CALL(*mockScene->mockEntityRepository, instantiate(testing::Eq(std::string_view(PARENT_ENTITY_NAME))))
			.WillOnce(testing::Return(parentEntity));
		EXPECT_CALL(*mockScene->mockEntityRepository, getEntityPtrById(testing::Eq(PARENT_ENTITY_ID)))
			.WillOnce(testing::Return(parentEntity));

		// These are called from within the Entity
		EXPECT_CALL(*mockScene->mockSceneGraphManager, getEntityPtrById(testing::Eq(PARENT_ENTITY_ID)))
			.WillOnce(testing::Return(parentEntity));
		EXPECT_CALL(*mockScene->mockSceneGraphManager, getEntityPtrById(testing::Eq(CHILD_ENTITY_1_ID)))
			.WillOnce(testing::Return(childEntity));
	}

	{
		const auto parentEntity = sceneGraphManager->instantiate(PARENT_ENTITY_NAME);
		ASSERT_NE(parentEntity.get(), nullptr);
		const auto childEntity = sceneGraphManager->instantiate(CHILD_ENTITY_1_NAME, *parentEntity);
		ASSERT_NE(childEntity.get(), nullptr);
		ASSERT_TRUE(childEntity->hasParent());

		parentEntity->setPosition({ 1.0f, 2.0f, 3.0f });
		sceneGraphManager->tick();
		EXPECT_FLOAT_EQ(childEntity->worldPosition().x, 1.0f);
		EXPECT_FLOAT_EQ(childEntity->worldPosition().y, 2.0f);
		EXPECT_FLOAT_EQ(childEntity->worldPosition().z, 3.0f);
	}

	sceneGraphManager.reset();
	mockScene->shutdown();
}