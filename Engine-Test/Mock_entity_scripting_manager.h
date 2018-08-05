#pragma once

#include <gmock/gmock.h>

#include "..\Engine\Entity_scripting_manager.h"

namespace oe_test {
	class Mock_entity_scripting_manager : public oe::IEntity_scripting_manager {
	public:
		Mock_entity_scripting_manager(oe::Scene& scene) : IEntity_scripting_manager(scene) {}

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

		// Manager_tickable implementation
		MOCK_METHOD0(tick, void());
	};
}