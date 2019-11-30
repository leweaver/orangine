#pragma once

#include "Component.h"

namespace oe {
	class Entity;

	class Test_component : public Component
	{
		DECLARE_COMPONENT_TYPE;

		std::string _testString;
		SSE::Vector3 _speed;

	public:
		Test_component(Entity& entity)
			: Component(entity)
			, _speed(SSE::Vector3(0))
		{			
		}

		void setSpeed(const SSE::Vector3& speed) { _speed = speed; }
		const SSE::Vector3& getSpeed() const { return _speed; }
	};
}