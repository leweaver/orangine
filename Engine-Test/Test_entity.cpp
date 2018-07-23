#include "pch.h"

#include "Mocks.h"

#include <string>

using namespace std::string_literals;
using namespace oe;
using namespace fakeit;

class Entity_transforms_test : public ::testing::Test {
protected:
	void SetUp() override
	{	
		//Method(sgmMock, instantiate) = _testEntity;

		_scene = std::make_unique<Mock_scene>();
	}

	//Mock<IScene_graph_manager> sgmMock;
	std::unique_ptr<Scene> _scene;
	std::shared_ptr<Entity> _testEntity;


};

TEST_F(Entity_transforms_test, worldTransformIsRH) 
{
	//const auto entity = sgmMock.get().instantiate("ASDF"s);
	//EXPECT_EQ(entity.get(), nullptr);
}