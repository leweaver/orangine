#pragma once

#include "OeCore/Texture.h"

#include <memory>

namespace oe {
/*
 * This is a simple shadow map texture pool, that allows creation of shadow map depth textures
 * that exist in a single Array Texture2D.
 *
 * Each shadow map will exist on a unique slice of the array texture.
 */
class Shadow_map_texture_pool {
 public:
  virtual ~Shadow_map_texture_pool() {}

  /// Must be called by the owner of this pool
  virtual void createDeviceDependentResources() = 0;

  /// Must be called by the owner of this pool
  virtual void destroyDeviceDependentResources() = 0;

  // If the pool is waiting for textures to be returned, this will throw.
  // If the pool is exhausted (all textures are allocated) then this will return nullptr.
  virtual std::shared_ptr<Texture> borrowTexture() = 0;

  // once a texture has been returned to the pool, no more can be borrowed until
  // all have been returned back to the pool.
  virtual void returnTexture(std::shared_ptr<Texture> shadowMap) = 0;

  // Shader resource view that can be used when sampling the shadow map depth
  virtual std::shared_ptr<Texture> shadowMapDepthTextureArray() = 0;

  // Shader resource view that can be used when sampling the shadow map stencil
  virtual std::shared_ptr<Texture> shadowMapStencilTextureArray() = 0;
};
} // namespace oe
