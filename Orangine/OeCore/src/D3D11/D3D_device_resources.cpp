#include <OeCore/DeviceResources.h>
#include <OeCore/EngineUtils.h>

#include "D3D12_device_resources.h"

#include <CommonStates.h>

using namespace DirectX;
using namespace oe;

D3D12_device_resources::D3D12_device_resources(DX::DeviceResources& deviceResources)
    : _deviceResources(deviceResources), _commonStates(nullptr), _window(nullptr) {}

void D3D12_device_resources::createDeviceDependentResources() {
  _commonStates = std::make_unique<CommonStates>(_deviceResources.GetD3DDevice());
}

void D3D12_device_resources::destroyDeviceDependentResources() { _commonStates.reset(); }

DirectX::CommonStates& D3D12_device_resources::commonStates() const {
  if (!_commonStates) {
    OE_THROW(std::runtime_error("device dependent resources not available (commonStates)"));
  }
  return *_commonStates.get();
}