#pragma once

#include <OeCore/IInput_manager.h>

#include "Mouse.h"

namespace oe::pipeline_d3d12 {
class D3D12_input_manager : public IInput_manager
    , public Manager_base
    , public Manager_windowDependent
    , public Manager_tickable
    , public Manager_windowsMessageProcessor {
 public:
  explicit D3D12_input_manager(IUser_interface_manager& userInterfaceManager);

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_tickable implementation
  void tick() override;

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(HWND window, int width, int height) override;
  void destroyWindowSizeDependentResources() override;

  // Manager_windowsMessageProcessor implementation
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

  // Mouse functions
  std::weak_ptr<Mouse_state> getMouseState() const override;

 private:

  static std::string _name;

  std::shared_ptr<Mouse_state> _mouseState;
  IUser_interface_manager& _userInterfaceManager;
  DirectX::Mouse::ButtonStateTracker _buttonStateTracker;
  std::unique_ptr<DirectX::Mouse> _mouse;
};
}
