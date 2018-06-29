#pragma once
#include "Manager_base.h"

namespace DX {
	class DeviceResources;
}

namespace oe {

class Input_manager : 
	public Manager_base, 
	public Manager_windowDependent,
	public Manager_tickable,
	public Manager_windowsMessageProcessor {
public:

	struct Mouse_state {
		enum class Button_state
		{
			UP = 0,         // Button is up
			HELD = 1,       // Button is held down
			RELEASED = 2,   // Button was just released
			PRESSED = 3,    // Buton was just pressed
		};

		POINT absolutePosition;
		POINT deltaPosition;

		Button_state left;
		Button_state middle;
		Button_state right;
	};

	explicit Input_manager(Scene &scene, DX::DeviceResources& deviceResources);

	// Manager_base implementation
	void initialize() override;
	void shutdown() override;

	// Manager_tickable implementation
	void tick() override;

	// Manager_windowDependent implementation
	void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
	void destroyWindowSizeDependentResources() override;

	// Manager_windowsMessageProcessor implementation
	void processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

	// Mouse functions
	std::weak_ptr<Mouse_state> mouseState() const;
	
private:
	class Impl;
	std::unique_ptr<Impl> _impl;
};

}
