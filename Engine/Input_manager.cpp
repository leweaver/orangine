#include "pch.h"
#include "Input_manager.h"

#include "DeviceResources.h"
#include "Scene.h"
#include <Mouse.h>

using namespace oe;

class Input_manager::Impl {
public:

	void tick()
	{
		if (mouse && mouse->IsConnected()) {
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
	: IInput_manager(scene)
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

bool Input_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_scene.manager<IUser_interface_manager>().mouseCaptured())
	    _impl->mouse->ProcessMessage(message, wParam, lParam);

    return false;
}

std::weak_ptr<Input_manager::Mouse_state> Input_manager::mouseState() const
{
	return std::weak_ptr<Mouse_state>(_impl->mouseState);
}

