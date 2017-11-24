#pragma once

#include "Component.h"

namespace OE {
DECLARE_COMPONENT(TestComponent)

	void Init() override;
	void Update() override;
};
}