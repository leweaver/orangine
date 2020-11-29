#pragma once

#include "Manager_base.h"
#include "Shadow_map_texture_pool.h"

#include "Texture.h"

namespace oe {
class Scene;

class ITexture_manager
    : public Manager_base {
 public:
  explicit ITexture_manager(Scene& scene) : Manager_base(scene) {}

  virtual std::shared_ptr<Texture> createTextureFromBuffer(
      uint32_t stride,
      uint32_t buffer_size,
      std::unique_ptr<uint8_t>& buffer) = 0;

  virtual std::shared_ptr<Texture> createTextureFromFile(const std::wstring& fileName) = 0;
  virtual std::shared_ptr<Texture> createTextureFromFile(const std::wstring& fileName, const Sampler_descriptor& samplerDescriptor) = 0;

  // The 32 bit depth/stencil texture, in R24 UNORM / X8 TYPELESS format.
  virtual std::shared_ptr<Texture> createDepthTexture() = 0;

  // A 32 bit RGBA render target texture that can be drawn to, and read by other materials.
  virtual std::shared_ptr<Texture> createRenderTargetTexture(int width, int height) = 0;

  // A texture that points to the main view
  virtual std::shared_ptr<Texture> createRenderTargetViewTexture() = 0;

  // Used to allocate textures for shadowmapping
  virtual std::unique_ptr<Shadow_map_texture_pool> createShadowMapTexturePool(
      uint32_t maxDimension,
      uint32_t textureArraySize) = 0;

  virtual void load(Texture& texture) = 0;
  virtual void unload(Texture& texture) = 0;
};
} // namespace oe