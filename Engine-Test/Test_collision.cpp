#include "pch.h"

#include "..\Engine\Collision.h"

#include <SimpleMath.h>

using namespace DirectX;
using namespace std::string_literals;
using namespace oe;

class Collision_tests : public ::testing::Test {
protected:

	static BoundingFrustumRH createForwardCamera()
	{
		const auto projMatrixRH = XMMatrixPerspectiveFovRH(
			XMConvertToRadians(70),
			640.0f / 480.0f,
			0.1f,
			10.0f);
		return BoundingFrustumRH(projMatrixRH);
	}
};

TEST_F(Collision_tests, frustumSphere)
{
	const auto frustum = createForwardCamera();

	const auto containedSphere = BoundingSphere(SimpleMath::Vector3::Forward * 5.0f, 1.0f);
	const auto nearSphere = BoundingSphere(SimpleMath::Vector3::Forward * 0.1f, 0.1f);
	const auto farSphere = BoundingSphere(SimpleMath::Vector3::Forward * 10.0f, 0.1f);
	const auto outsideSphere = BoundingSphere(SimpleMath::Vector3::Backward * 5.0f, 1.0f);

	EXPECT_EQ(ContainmentType::CONTAINS, frustum.Contains(containedSphere));
	EXPECT_EQ(ContainmentType::INTERSECTS, frustum.Contains(nearSphere));
	EXPECT_EQ(ContainmentType::INTERSECTS, frustum.Contains(farSphere));
	EXPECT_EQ(ContainmentType::DISJOINT, frustum.Contains(outsideSphere));
}