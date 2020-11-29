#pragma once

#include <CommonStates.h>

#include <OeCore/DeviceResources.h>
#include <OeCore/IDevice_repository.h>

namespace oe::internal {
class D3D_device_repository : public IDevice_repository {
 public:
  D3D_device_repository(DX::DeviceResources& deviceResources);

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
