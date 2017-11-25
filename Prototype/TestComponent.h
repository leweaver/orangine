#pragma once

#include "Component.h"

namespace OE {
//DECLARE_COMPONENT(TestComponent)
class TestComponent : public Component
{
	std::string m_testString;

public:
	void Initialize() override;
	void Update() override;
};
}