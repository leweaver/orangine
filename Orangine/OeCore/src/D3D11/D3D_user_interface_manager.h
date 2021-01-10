#pragma once

#include "OeCore/IUser_interface_manager.h"

#include "D3D_device_repository.h"

namespace oe::internal {
class D3D_user_interface_manager : public IUser_interface_manager {
 public:
  D3D_user_interface_manager(
      Scene& scene,
      std::shared_ptr<D3D_device_repository> device_repository);

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
  std::shared_ptr<D3D_device_repository> _deviceRepository;
  static std::string _name;

  HWND _window = nullptr;
  float _uiScale = 1.0f;
};
} // namespace oe::internal