#pragma once

#include <gmock/gmock.h>

#include "..\Engine\IInput_manager.h"

namespace oe_test {
	class Mock_input_manager : public oe::IInput_manager {
	public:
		Mock_input_manager(oe::Scene& scene) : IInput_manager(scene) {}

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

		// Manager_tickable implementation
		MOCK_METHOD0(tick, void());

		// Manager_windowDependent implementation
		MOCK_METHOD4(createWindowSizeDependentResources, void(DX::DeviceResources&, HWND, int, int));
		MOCK_METHOD0(destroyWindowSizeDependentResources, void());

		// Manager_windowsMessageProcessor implementation
		MOCK_METHOD3(processMessage, bool(UINT, WPARAM, LPARAM));

        // IInput_manager implementation
        MOCK_CONST_METHOD0(mouseState, std::weak_ptr<oe::IInput_manager::Mouse_state>());
	};
}