#include "pch.h"

#include "../Engine/Entity_render_manager.h"
#include "Mock_scene.h"
#include "Fake_entity_filter.h"

#include <DirectXCollision.h>
#include <string>

using namespace oe;
using namespace std::string_literals;
using namespace DirectX::SimpleMath;

class Renderer_tests : public ::testing::Test {
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
};

Quaternion quatFromNormal(const Vector3& normal)
{
	if (normal != Vector3::Forward)
	{
		Vector3 axis;
		if (normal == Vector3::Backward)
			axis = Vector3::Up;
		else
		{
			axis = Vector3::Forward.Cross(normal);
			axis.Normalize();
		}

		assert(normal.LengthSquared() != 0);
		float angle = acos(Vector3::Forward.Dot(normal) / normal.Length());
		return Quaternion::CreateFromAxisAngle(axis, angle);
	}
	return Quaternion::Identity;
}

void testAABBForEntities(const Vector3& normal, const Vector3& expectedCenter, const Vector3& expectedExtents)
{
	const auto mockScene = oe_test::Mock_scene::createInit();
	auto e1 = std::make_shared<Entity>(*mockScene, std::string("e1"), 1);
	e1->setPosition({ -1.0f, 1.0f, 0.0f });
	e1->setBoundSphere(DirectX::BoundingSphere(Vector3::Zero, 1.0f));
	e1->computeWorldTransform();

	auto e2 = std::make_shared<Entity>(*mockScene, std::string("e2"), 2);
	e2->setPosition({ 1.0f, 1.0f, 0.0f });
	e2->setBoundSphere(DirectX::BoundingSphere(Vector3::Zero, 1.0f));
	e2->computeWorldTransform();

	const std::vector<std::shared_ptr<Entity>> entities = { e1, e2 };
	const auto filter = oe_test::Fake_entity_filter(entities);

	const auto aabb = Entity_render_manager::aabbForEntities(filter, quatFromNormal(normal));

	EXPECT_NEAR(aabb.Center.x, expectedCenter.x, 1e-6f);
	EXPECT_NEAR(aabb.Center.y, expectedCenter.y, 1e-6f);
	EXPECT_NEAR(aabb.Center.z, expectedCenter.z, 1e-6f);

	EXPECT_NEAR(aabb.Extents.x, expectedExtents.x, 1e-6);
	EXPECT_NEAR(aabb.Extents.y, expectedExtents.y, 1e-6);
	EXPECT_NEAR(aabb.Extents.z, expectedExtents.z, 1e-6);

	mockScene->shutdown();
}

TEST_F(Renderer_tests, aabbForEntitiesDown)
{
	testAABBForEntities({ 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 2.0f, 1.0f, 1.0f });
}

TEST_F(Renderer_tests, aabbForEntitiesUp)
{
	testAABBForEntities({ 0.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 2.0f, 1.0f, 1.0f });
}

TEST_F(Renderer_tests, aabbForEntitiesLeft)
{
	testAABBForEntities({ -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 2.0f });
}

TEST_F(Renderer_tests, aabbForEntitiesRight)
{
	testAABBForEntities({  1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 2.0f });
}

TEST_F(Renderer_tests, aabbForEntitiesForward)
{
	testAABBForEntities({ 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 2.0f, 1.0f, 1.0f });
}

TEST_F(Renderer_tests, aabbForEntitiesBackward)
{
	testAABBForEntities({ 0.0f, 0.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 2.0f, 1.0f, 1.0f });
}
