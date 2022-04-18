#include "D3D12_lighting_manager.h"
#include "D3D12_device_resources.h"

#include <OeCore/ITexture_manager.h>

using namespace oe::pipeline_d3d12;

std::string D3D12_lighting_manager::_name;// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<>
void oe::create_manager(
        Manager_instance<ILighting_manager>& out, ITexture_manager& textureManager,
        D3D12_device_resources& deviceResources)
{
  out = Manager_instance<ILighting_manager>(std::make_unique<D3D12_lighting_manager>(textureManager, deviceResources));
}

void D3D12_lighting_manager::initStatics()
{
  D3D12_lighting_manager::_name = "D3D12_lighting_manager";
}

void D3D12_lighting_manager::destroyStatics() {}

void D3D12_lighting_manager::addEnvironmentVolume(Environment_volume& volume)
{
  _environment = volume;
}

void D3D12_lighting_manager::setCurrentVolumeEnvironmentLighting(const Vector3& position)
{
  if (_deviceDependent._renderLightData_lit == nullptr) {
    return;
  }

  if (_environment.environmentIbl.skyboxTexture.get() != _environmentIbl.skyboxTexture.get()) {
    _environmentIbl = _environment.environmentIbl;

    _textureManager.load(*_environmentIbl.iblBrdfTexture);
    _textureManager.load(*_environmentIbl.iblDiffuseTexture);
    _textureManager.load(*_environmentIbl.iblSpecularTexture);

    // Force reload of textures
    _deviceDependent._renderLightData_lit->setEnvironmentIblMap(nullptr, nullptr, nullptr);
  }

  if (_environmentIbl.iblDiffuseTexture && !_deviceDependent._renderLightData_lit->environmentMapDiffuse()) {
    _deviceDependent._renderLightData_lit->setEnvironmentIblMap(
            _environmentIbl.iblBrdfTexture, _environmentIbl.iblDiffuseTexture, _environmentIbl.iblSpecularTexture);
  }
}
void D3D12_lighting_manager::createDeviceDependentResources()
{
  ID3D12Device6* device = _deviceResources.GetD3DDevice();
  _deviceDependent = {
          std::make_unique<D3D12_render_light_data<0>>(device), std::make_unique<D3D12_render_light_data<8>>(device)};
}
void D3D12_lighting_manager::destroyDeviceDependentResources()
{
  _deviceDependent = {};
}
