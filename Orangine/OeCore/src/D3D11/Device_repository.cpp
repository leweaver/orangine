#include "pch.h"

#include "OeCore/DeviceResources.h"

#include "Device_repository.h"

#include <CommonStates.h>

using namespace DirectX;
using namespace oe;
using namespace internal;

Device_repository::Device_repository(DX::DeviceResources& deviceResources)
    : _deviceResources(deviceResources), _commonStates(nullptr), _window(nullptr) {}

void Device_repository::createDeviceDependentResources() {
  _commonStates = std::make_unique<CommonStates>(_deviceResources.GetD3DDevice());
}

void Device_repository::destroyDeviceDependentResources() { _commonStates.reset(); }
