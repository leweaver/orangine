#include "pch.h"
#include "Input_manager.h"

#include "DeviceResources.h"
#include "Scene.h"
#include <Mouse.h>

using namespace oe;
using namespace internal;

std::string Input_manager::_name = "Input_manager";

template<>
IInput_manager* oe::create_manager(Scene& scene)
{
    return new Input_manager(scene);
}

// DirectX Singletons
Input_manager::Input_manager(Scene& scene)
	: IInput_manager(scene)
    , _mouse(nullptr)
    , _mouseState(std::make_shared<Mouse_state>())
{
}

void Input_manager::initialize()
{
}

void Input_manager::shutdown()
{
	_mouse.reset();
}

const std::string& Input_manager::name() const
{
    return _name;
}

void Input_manager::tick()
{
    if (_mouse && _mouse->IsConnected()) {
        const auto state = _mouse->GetState();
        _buttonStateTracker.Update(state);

        auto& mouseState = *_mouseState;
        mouseState.left = static_cast<Mouse_state::Button_state>(_buttonStateTracker.leftButton);
        mouseState.right = static_cast<Mouse_state::Button_state>(_buttonStateTracker.rightButton);
        mouseState.middle = static_cast<Mouse_state::Button_state>(_buttonStateTracker.middleButton);

        const auto lastPosition = mouseState.absolutePosition;
        mouseState.absolutePosition.x = state.x;
        mouseState.absolutePosition.y = state.y;
        mouseState.deltaPosition.x = state.x - lastPosition.x;
        mouseState.deltaPosition.y = state.y - lastPosition.y;

        mouseState.scrollWheelDelta = state.scrollWheelValue;
        _mouse->ResetScrollWheelValue();
    }
}

void Input_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND window, int /*width*/, int /*height*/)
{
	_mouse = std::make_unique<DirectX::Mouse>();
	_mouse->SetWindow(window);
}

void Input_manager::destroyWindowSizeDependentResources()
{
	_mouse = std::unique_ptr<DirectX::Mouse>();
}

bool Input_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!_scene.manager<IUser_interface_manager>().mouseCaptured())
	    _mouse->ProcessMessage(message, wParam, lParam);

    return false;
}

std::weak_ptr<Input_manager::Mouse_state> Input_manager::mouseState() const
{
	return std::weak_ptr<Mouse_state>(_mouseState);
}

