#pragma once

#include "Component.h"

namespace OE {
	class Entity;

	class TestComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		std::string m_testString;
		DirectX::SimpleMath::Vector3 m_speed;

	public:
		TestComponent()
			: m_speed(DirectX::SimpleMath::Vector3::Zero)
		{			
		}

		void SetSpeed(const DirectX::SimpleMath::Vector3 &speed) { m_speed = speed; }
		const DirectX::SimpleMath::Vector3 &GetSpeed() const { return m_speed; }
	};
}