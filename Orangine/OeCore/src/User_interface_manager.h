#pragma once

#include "OeCore/IUser_interface_manager.h"

#include "D3D11/Device_repository.h"

namespace oe::internal {
class User_interface_manager : public IUser_interface_manager {
 public:
  User_interface_manager(Scene& scene, std::shared_ptr<Device_repository> device_repository);

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(HWND window, int width, int height) override;
  void destroyWindowSizeDependentResources() override;

  // Manager_windowsMessageProcessor implementation
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

  // IUser_interface_manager implementation
  void render() override;
  bool keyboardCaptured() override;
  bool mouseCaptured() override;
  void preInit_setUIScale(float uiScale) override { _uiScale = uiScale; }

 private:
  std::shared_ptr<Device_repository> _deviceRepository;
  static std::string _name;

  HWND _window = nullptr;
  float _uiScale = 1.0f;
};
} // namespace oe::internal