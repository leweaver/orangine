#include "pch.h"
#include "TestComponent.h"
#include "Constants.h"
#include "Entity.h"
#include "Scene.h"
#include <cmath>

using namespace DirectX;
using namespace OE;

DEFINE_COMPONENT_TYPE(TestComponent);
/*
void TestComponent::Initialize()
{
	m_testString = std::string("asdf");
}

void TestComponent::Update()
{
	float elapsedTime = static_cast<float>(m_entity.GetScene().EntityManager().ElapsedTime());

	float animTimeRoll = std::fmod(elapsedTime * XMVectorGetX(m_speed) * XM_2PI, XM_2PI);
	float animTimePitch = std::fmod(elapsedTime * XMVectorGetY(m_speed) * XM_2PI, XM_2PI);
	float animTimeYaw = std::fmod(elapsedTime * XMVectorGetZ(m_speed) * XM_2PI, XM_2PI);

	m_entity.SetLocalRotation(DirectX::XMQuaternionRotationRollPitchYaw(
		animTimeRoll,
		animTimePitch,
		animTimeYaw));

}
*/
