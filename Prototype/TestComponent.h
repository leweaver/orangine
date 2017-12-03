#pragma once

#include "Component.h"
#include "Constants.h"

namespace OE {
	class Entity;

	class TestComponent : public Component
	{
		std::string m_testString;
		DirectX::XMVECTOR m_speed;

	public:
		TestComponent(Entity& entity)
			: Component(entity)
			, m_speed(Math::VEC_ZERO)
		{			
		}
		void Initialize() override;
		void Update() override;

		void SetSpeed(const DirectX::FXMVECTOR &speed) { m_speed = speed; }
	};
}