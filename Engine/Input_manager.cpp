#include "pch.h"
#include "Input_manager.h"

#include "DeviceResources.h"
#include <Mouse.h>

using namespace oe;

class Input_manager::Impl {
public:

	void tick()
	{
		if (mouse->IsConnected()) {
			const auto state = mouse->GetState();
			buttonStateTracker.Update(state);

			mouseState->left = static_cast<Mouse_state::Button_state>(buttonStateTracker.leftButton);
			mouseState->right = static_cast<Mouse_state::Button_state>(buttonStateTracker.rightButton);
			mouseState->middle = static_cast<Mouse_state::Button_state>(buttonStateTracker.middleButton);

			const auto lastPosition = mouseState->absolutePosition;
			mouseState->absolutePosition.x = state.x;
			mouseState->absolutePosition.y = state.y;
			mouseState->deltaPosition.x = state.x - lastPosition.x;
			mouseState->deltaPosition.y = state.y - lastPosition.y;

			mouseState->scrollWheelDelta = state.scrollWheelValue;
			mouse->ResetScrollWheelValue();
		}
	}

	DirectX::Mouse::ButtonStateTracker buttonStateTracker;
	std::unique_ptr<DirectX::Mouse> mouse;
	std::shared_ptr<Mouse_state> mouseState = std::make_shared<Mouse_state>();
};

// DirectX Singletons
Input_manager::Input_manager(Scene& scene, DX::DeviceResources& deviceResources)
	: Manager_base(scene)
	, _impl(std::make_unique<Impl>())
{
}

void Input_manager::initialize()
{
}

void Input_manager::shutdown()
{
	_impl->mouse.reset();
}

void Input_manager::tick()
{
	_impl->tick();
}

void Input_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND window, int /*width*/, int /*height*/)
{
	_impl->mouse = std::make_unique<DirectX::Mouse>();
	_impl->mouse->SetWindow(window);
}

void Input_manager::destroyWindowSizeDependentResources()
{
	_impl->mouse = std::unique_ptr<DirectX::Mouse>();
}

void Input_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	_impl->mouse->ProcessMessage(message, wParam, lParam);
}

std::weak_ptr<Input_manager::Mouse_state> Input_manager::mouseState() const
{
	return std::weak_ptr<Mouse_state>(_impl->mouseState);
}

