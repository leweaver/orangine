#pragma once

#include <gmock/gmock.h>

#include "..\Engine\IEntity_scripting_manager.h"

namespace oe_test {
	class Mock_entity_scripting_manager : public oe::IEntity_scripting_manager {
	public:
		Mock_entity_scripting_manager(oe::Scene& scene) : IEntity_scripting_manager(scene) {}

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

		// Manager_tickable implementation
		MOCK_METHOD0(tick, void());

        // IEntity_scripting_manager implementation
        MOCK_METHOD0(renderImGui, void());
        MOCK_METHOD1(execute, void(const std::string& command));
        MOCK_METHOD2(commandSuggestions, bool(const std::string& command, std::vector<std::string>& suggestions));
	};
}