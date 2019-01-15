#pragma once
#include "Manager_base.h"

namespace DX {
	class DeviceResources;
}

namespace oe {

class IInput_manager :
	public Manager_base,
	public Manager_windowDependent,
	public Manager_tickable,
	public Manager_windowsMessageProcessor {
public:
	explicit IInput_manager(Scene& scene) : Manager_base(scene) {}

	struct Mouse_state {
		enum class Button_state
		{
			UP = 0,         // Button is up
			HELD = 1,       // Button is held down
			RELEASED = 2,   // Button was just released
			PRESSED = 3,    // Buton was just pressed
		};

		POINT absolutePosition = { 0, 0 };
		POINT deltaPosition = { 0, 0 };

		Button_state left = Button_state::UP;
		Button_state middle = Button_state::UP;
		Button_state right = Button_state::UP;
		int scrollWheelDelta = 0;
	};

	// Mouse functions
	virtual std::weak_ptr<Mouse_state> mouseState() const = 0;
};

class Input_manager : public IInput_manager {
public:
	
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
	bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

	// Mouse functions
	std::weak_ptr<Mouse_state> mouseState() const override;
	
private:
	class Impl;
	std::unique_ptr<Impl> _impl;
};

}
