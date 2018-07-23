#pragma once

#include "Component.h"

namespace oe {
	class Entity;

	class Test_component : public Component
	{
		DECLARE_COMPONENT_TYPE;

		std::string _testString;
		DirectX::SimpleMath::Vector3 _speed;

	public:
		Test_component()
			: _speed(DirectX::SimpleMath::Vector3::Zero)
		{			
		}

		void setSpeed(const DirectX::SimpleMath::Vector3& speed) { _speed = speed; }
		const DirectX::SimpleMath::Vector3& getSpeed() const { return _speed; }
	};
}