#pragma once

#include "Component.h"
#include "Constants.h"

namespace OE {
	class Entity;

	class TestComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		std::string m_testString;
		DirectX::XMVECTOR m_speed;

	public:
		TestComponent()
			: m_speed(Math::VEC_ZERO)
		{			
		}

		void SetSpeed(const DirectX::FXMVECTOR &speed) { m_speed = speed; }
		const DirectX::XMVECTOR &GetSpeed() const { return m_speed; }
	};
}