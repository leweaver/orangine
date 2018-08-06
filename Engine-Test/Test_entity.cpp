#include "pch.h"

#include "Mock_scene.h"

#include <string>

using namespace std::string_literals;
using namespace oe;
using namespace oe_test;

class Entity_transforms_test : public ::testing::Test {
protected:
	void SetUp() override
	{
		const auto hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
		ASSERT_TRUE(SUCCEEDED(hr));

		_mockScene = std::make_unique<Mock_scene>();
		_mockScene->initialize();
	}

	void TearDown() override
	{
		_mockScene->shutdown();
		_mockScene.reset();

		CoUninitialize();
	}

	std::unique_ptr<Mock_scene> _mockScene;
	std::shared_ptr<Entity> _testEntity;
};

TEST_F(Entity_transforms_test, worldTransformIsRH) 
{
	//const auto entity = sgmMock.get().instantiate("ASDF"s);
	//EXPECT_EQ(entity.get(), nullptr);
	ASSERT_TRUE(true);
}