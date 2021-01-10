#pragma once

#include <CommonStates.h>

#include <OeCore/IDevice_resources.h>

namespace oe {
class D3D_device_repository : public IDevice_resources {
 public:
  D3D_device_repository();
  virtual ~D3D_device_repository() = default;

  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  DirectX::CommonStates& commonStates() const;
  DX::DeviceResources& deviceResources() const { return _deviceResources; }

 private:
  DX::DeviceResources& _deviceResources;
  std::unique_ptr<DirectX::CommonStates> _commonStates;
  HWND _window;
};
} // namespace oe::internal
