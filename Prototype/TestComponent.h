#pragma once

#include "Component.h"

namespace OE {
//DECLARE_COMPONENT(TestComponent)
class TestComponent : public Component
{
public:
	virtual void Init();
	virtual void Update();
};
}