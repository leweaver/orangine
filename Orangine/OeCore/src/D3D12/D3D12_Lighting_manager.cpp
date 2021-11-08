#include "pch.h"

#include "D3D12_lighting_manager.h"

#include <OeCore/ITexture_manager.h>

using namespace oe;

std::string D3D12_lighting_manager::_name; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template <>
oe::ILighting_manager* oe::create_manager(
        Scene& scene,
        ITexture_manager& textureManager) {
  return new D3D12_lighting_manager(scene, textureManager);
}

void D3D12_lighting_manager::initStatics() {
  D3D12_lighting_manager::_name = "D3D12_lighting_manager";
}

void D3D12_lighting_manager::destroyStatics() {}

void D3D12_lighting_manager::addEnvironmentVolume(Environment_volume& volume) {
  _environment = volume;
}

void D3D12_lighting_manager::setCurrentVolumeEnvironmentLighting(const Vector3& position) {
  if (_environment.environmentIbl.skyboxTexture.get() != _environmentIbl.skyboxTexture.get()) {
    _environmentIbl = _environment.environmentIbl;

    _textureManager.load(*_environmentIbl.iblBrdfTexture);
    _textureManager.load(*_environmentIbl.iblDiffuseTexture);
    _textureManager.load(*_environmentIbl.iblSpecularTexture);

    // Force reload of textures
    _renderLightData_lit->setEnvironmentIblMap(nullptr, nullptr, nullptr);
  }

  if (_environmentIbl.iblDiffuseTexture && !_renderLightData_lit->environmentMapDiffuse()) {
    _renderLightData_lit->setEnvironmentIblMap(
            _environmentIbl.iblBrdfTexture,
            _environmentIbl.iblDiffuseTexture,
            _environmentIbl.iblSpecularTexture);
  }
}
