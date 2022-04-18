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
  // TODO:
  //_texturePool = _textureManager.createShadowMapTexturePool(256, 8);
  //_texturePool->createDeviceDependentResources();
}

void Shadowmap_manager::destroyDeviceDependentResources() {
  if (_texturePool) {
    _texturePool->destroyDeviceDependentResources();
    _texturePool.reset();
  }
}

std::shared_ptr<Texture> Shadowmap_manager::borrowTexture() {
  if (!verifyTexturePool()) {
    return nullptr;
  }

  auto texture = _texturePool->borrowTexture();
  if (!texture->isValid()) {
    _textureManager.load(*texture);
  }

  OE_CHECK(texture->isValid());
  return texture;
}

void Shadowmap_manager::returnTexture(std::shared_ptr<Texture> shadowMap) {
  if (verifyTexturePool()) {
    _texturePool->returnTexture(move(shadowMap));
  }
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapDepthTextureArray() {
  return verifyTexturePool() ? _texturePool->shadowMapDepthTextureArray() : nullptr;
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapStencilTextureArray() {
  return verifyTexturePool() ? _texturePool->shadowMapStencilTextureArray() : nullptr;
}

bool Shadowmap_manager::verifyTexturePool() const {
  if (!_texturePool) {
    LOG(WARNING) << "Invalid attempt to call shadowmap methods when no device is available";
    return false;
  }
  return true;
}
