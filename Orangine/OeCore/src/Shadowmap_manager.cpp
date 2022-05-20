#include <OeCore/EngineUtils.h>

#include "Shadowmap_manager.h"

using namespace oe;

std::string Shadowmap_manager::_name = "Shadowmap_manager";

template<> void oe::create_manager(Manager_instance<IShadowmap_manager>& out, ITexture_manager& textureManager)
{
  out = Manager_instance<IShadowmap_manager>(std::make_unique<Shadowmap_manager>(textureManager));
}

const std::string& Shadowmap_manager::name() const { return _name; }

void Shadowmap_manager::initialize()
{
  _texturePool = _textureManager.createShadowMapTexturePool(256, 8);
}

void Shadowmap_manager::shutdown()
{
  if (_texturePool) {
    _textureManager.release(_texturePool);
  }
}

std::shared_ptr<Texture> Shadowmap_manager::borrowTextureSlice() {
  if (!verifyTexturePool()) {
    return nullptr;
  }

  auto texture = _texturePool->borrowTextureSlice();
  if (!texture->isValid()) {
    _textureManager.load(*texture);
  }

  OE_CHECK(texture->isValid());
  return texture;
}

void Shadowmap_manager::returnTextureSlice(std::shared_ptr<Texture> shadowMap) {
  if (verifyTexturePool()) {
    _texturePool->returnTextureSlice(move(shadowMap));
  }
}

std::shared_ptr<Texture> Shadowmap_manager::getShadowMapTextureArray() {
  return verifyTexturePool() ? _texturePool->getShadowMapTextureArray() : nullptr;
}

bool Shadowmap_manager::verifyTexturePool() const {
  if (!_texturePool) {
    LOG(WARNING) << "Invalid attempt to call shadowmap methods when no device is available";
    return false;
  }
  return true;
}
