﻿#include "pch.h"

#include "Shadowmap_manager.h"

#include "OeCore/Scene.h"

using namespace oe;

std::string Shadowmap_manager::_name = "Shadowmap_manager";

template <>
IShadowmap_manager* oe::create_manager(Scene& scene, ITexture_manager& textureManager) {
  return new Shadowmap_manager(scene, textureManager);
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
