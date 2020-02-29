#include "pch.h"

#include "D3D11/Device_repository.h"
#include "OeCore/Scene.h"
#include "OeCore/Shadow_map_texture_pool.h"
#include "Shadowmap_manager.h"

using namespace oe;
using namespace internal;

std::string Shadowmap_manager::_name = "Shadowmap_manager";

template <>
IShadowmap_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<Device_repository>& deviceRepository) {
  return new Shadowmap_manager(scene, deviceRepository);
}

const std::string& Shadowmap_manager::name() const { return _name; }

void Shadowmap_manager::createDeviceDependentResources() {
  _texturePool = std::make_unique<Shadow_map_texture_pool>(256, 8, _deviceRepository);
  _texturePool->createDeviceDependentResources();
}

void Shadowmap_manager::destroyDeviceDependentResources() {
  if (_texturePool) {
    _texturePool->destroyDeviceDependentResources();
    _texturePool.reset();
  }
}

std::unique_ptr<Shadow_map_texture_array_slice> Shadowmap_manager::borrowTexture() {
  verifyTexturePool();
  return _texturePool->borrowTexture();
}

void Shadowmap_manager::returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap) {
  verifyTexturePool();
  _texturePool->returnTexture(move(shadowMap));
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapDepthTextureArray() {
  verifyTexturePool();
  return _texturePool->shadowMapDepthTextureArray();
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapStencilTextureArray() {
  verifyTexturePool();
  return _texturePool->shadowMapStencilTextureArray();
}

void Shadowmap_manager::verifyTexturePool() const {
  if (!_texturePool) {
    OE_THROW(
        std::logic_error("Invalid attempt to call shadowmap methods when no device is available"));
  }
}
