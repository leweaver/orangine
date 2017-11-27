#pragma once

#include "Component.h"

namespace OE {
	class Entity;

	class TestComponent : public Component
	{
		std::string m_testString;

	public:
		TestComponent(Entity& entity)
			: Component(entity)
		{			
		}
		void Initialize() override;
		void Update() override;
	};
}