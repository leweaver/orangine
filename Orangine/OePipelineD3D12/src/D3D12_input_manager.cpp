#include "D3D12_input_manager.h"

#include <Mouse.h>

using namespace oe::pipeline_d3d12;

std::string D3D12_input_manager::_name = "D3D12_input_manager";

template<> void oe::create_manager(Manager_instance<IInput_manager>& out, IUser_interface_manager& userInterfaceManager)
{
  out = Manager_instance<IInput_manager>(std::make_unique<D3D12_input_manager>(userInterfaceManager));
}

D3D12_input_manager::D3D12_input_manager(IUser_interface_manager& userInterfaceManager)
    : IInput_manager()
    , Manager_base()
    , Manager_windowDependent()
    , Manager_tickable()
    , Manager_windowsMessageProcessor()
    , _mouse(nullptr)
    , _mouseState(std::make_shared<Mouse_state>())
    , _userInterfaceManager(userInterfaceManager)
{}

void D3D12_input_manager::initialize() {}

void D3D12_input_manager::shutdown() { _mouse.reset(); }

const std::string& D3D12_input_manager::name() const { return _name; }

void D3D12_input_manager::tick() {
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

void D3D12_input_manager::createWindowSizeDependentResources(HWND window, int /*width*/, int /*height*/) {
  _mouse = std::make_unique<DirectX::Mouse>();
  _mouse->SetWindow(window);
}

void D3D12_input_manager::destroyWindowSizeDependentResources() {
  _mouse = std::unique_ptr<DirectX::Mouse>();
}

bool D3D12_input_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam) {
  if (!_userInterfaceManager.mouseCaptured()) {
    _mouse->ProcessMessage(message, wParam, lParam);
  }

  return false;
}

std::weak_ptr<D3D12_input_manager::Mouse_state> D3D12_input_manager::getMouseState() const {
  return {_mouseState};
}
