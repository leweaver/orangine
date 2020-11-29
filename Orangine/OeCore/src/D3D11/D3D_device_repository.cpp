#include "pch.h"

#include <OeCore/DeviceResources.h>
#include <OeCore/EngineUtils.h>

#include "D3D_device_repository.h"

#include <CommonStates.h>

using namespace DirectX;
using namespace oe;
using namespace internal;

D3D_device_repository::D3D_device_repository(DX::DeviceResources& deviceResources)
    : _deviceResources(deviceResources), _commonStates(nullptr), _window(nullptr) {}

void D3D_device_repository::createDeviceDependentResources() {
  _commonStates = std::make_unique<CommonStates>(_deviceResources.GetD3DDevice());
}

void D3D_device_repository::destroyDeviceDependentResources() { _commonStates.reset(); }

DirectX::CommonStates& D3D_device_repository::commonStates() const {
  if (!_commonStates) {
    OE_THROW(std::runtime_error("device dependent resources not available (commonStates)"));
  }
  return *_commonStates.get();
}