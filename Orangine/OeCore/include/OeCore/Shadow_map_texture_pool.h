#pragma once

#include "Shadow_map_texture.h"

#include <memory>
#include <stack>

namespace oe {
/*
 * This is a simple shadow map texture pool, that allows creation of shadow map depth textures
 * that exist in a single Array Texture2D.
 *
 * Each shadow map will exist on a unique slice of the array texture.
 */
class Shadow_map_texture_pool {
 public:
  explicit Shadow_map_texture_pool(uint32_t maxDimension, uint32_t textureArraySize);
  ~Shadow_map_texture_pool();

  void createDeviceDependentResources();
  void destroyDeviceDependentResources();

  // If the pool is waiting for textures to be returned, this will throw.
  // If the pool is exhausted (all textures are allocated) then this will return nullptr.
  std::unique_ptr<Shadow_map_texture_array_slice> borrowTexture();

  // once a texture has been returned to the pool, no more can be borrowed until
  // all have been returned back to the pool.
  void returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap);

  // Shader resource view that can be used when sampling the shadow map depth
  std::shared_ptr<Texture> shadowMapDepthTextureArray() { return _shadowMapDepthArrayTexture; };

  // Shader resource view that can be used when sampling the shadow map stencil
  std::shared_ptr<Texture> shadowMapStencilTextureArray() { return _shadowMapStencilArrayTexture; };

 protected:
  const uint32_t _dimension;
  const uint32_t _textureArraySize;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> _shadowMapArrayTexture2D;
  std::shared_ptr<Texture> _shadowMapDepthArrayTexture;
  std::shared_ptr<Texture> _shadowMapStencilArrayTexture;

  std::vector<std::unique_ptr<Shadow_map_texture_array_slice>> _shadowMaps;
};
} // namespace oe
