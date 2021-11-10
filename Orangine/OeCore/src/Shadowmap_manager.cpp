#include "pch.h"
#include <OeCore/EngineUtils.h>

#include "Shadowmap_manager.h"

using namespace oe;

std::string Shadowmap_manager::_name = "Shadowmap_manager";

template<> void oe::create_manager(Manager_instance<IShadowmap_manager>& out, ITexture_manager& textureManager)
{
  out = Manager_instance<IShadowmap_manager>(std::make_unique<Shadowmap_manager>(textureManager));
}

const std::string& Shadowmap_manager::name() const { return _name; }

void Shadowmap_manager::createDeviceDependentResources() {
  _texturePool = _textureManager.createShadowMapTexturePool(256, 8);
  _texturePool->createDeviceDependentResources();
}

void Shadowmap_manager::destroyDeviceDependentResources() {
  if (_texturePool) {
    _texturePool->destroyDeviceDependentResources();
    _texturePool.reset();
  }
}

std::shared_ptr<Texture> Shadowmap_manager::borrowTexture() {
  verifyTexturePool();
  auto texture = _texturePool->borrowTexture();
  if (!texture->isValid()) {
    _textureManager.load(*texture);
  }
  assert(texture->isValid());
  return texture;
}

void Shadowmap_manager::returnTexture(std::shared_ptr<Texture> shadowMap) {
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
