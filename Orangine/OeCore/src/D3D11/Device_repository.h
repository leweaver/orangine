﻿#pragma once

#include <CommonStates.h>

#include <OeCore/DeviceResources.h>
#include <OeCore/IDevice_repository.h>

namespace oe::internal {
class Device_repository : public IDevice_repository {
 public:
  Device_repository(DX::DeviceResources& deviceResources);

  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  DirectX::CommonStates& commonStates() const { return *_commonStates.get(); }
  DX::DeviceResources& deviceResources() const { return _deviceResources; }

 private:
  DX::DeviceResources& _deviceResources;
  std::unique_ptr<DirectX::CommonStates> _commonStates;
  HWND _window;
};
} // namespace oe::internal
