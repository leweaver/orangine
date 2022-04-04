#pragma once

#include <OeCore/ITexture_manager.h>
#include <OeCore/Texture.h>
#include "D3D12_device_resources.h"

namespace oe::pipeline_d3d12 {

class D3D12_texture : public Texture {
 public:
  explicit D3D12_texture(uint32_t extraFlags = 0) : Texture({0, 0}, extraFlags) {
    // Make sure that the 'Is_valid' flag wasn't passed in
    OE_CHECK(!isValid());
  }

  D3D12_texture(
          Vector2u dimension,
          uint32_t extraFlags,
          Microsoft::WRL::ComPtr<ID3D12Resource> resource)
      : Texture(
                dimension,
                extraFlags | static_cast<uint32_t>(
                                     resource != nullptr ? Texture_flags::Is_valid : Texture_flags::Invalid))
      , _resource(resource) {}

  void setResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
    if (resource == nullptr) {
      _flags = _flags & ~static_cast<uint32_t>(Texture_flags::Is_valid);
      _resource.Reset();
    } else {
      _flags = _flags | static_cast<uint32_t>(Texture_flags::Is_valid);
      _resource = resource;
    }
  }
  ID3D12Resource* getResource() const { return _resource.Get(); }

  virtual void load(D3D12_device_resources& deviceResources) = 0;
  virtual void unload() { setResource(nullptr); }

 private:
  Microsoft::WRL::ComPtr<ID3D12Resource> _resource;
};

class D3D12_texture_manager : public ITexture_manager, public Manager_base, public Manager_deviceDependent {
 public:
  D3D12_texture_manager() = default;

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override
  {
    return _name;
  }

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // ITexture_manager implementation
  std::shared_ptr<Texture>
  createTextureFromBuffer(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t>& buffer) override;
  std::shared_ptr<Texture> createTextureFromFile(const std::string& fileName) override;
  std::shared_ptr<Texture>
  createTextureFromFile(const std::string& fileName, const Sampler_descriptor& samplerDescriptor) override;

  std::shared_ptr<Texture> createDepthTexture() override;
  std::shared_ptr<Texture> createRenderTargetTexture(int width, int height) override;
  std::shared_ptr<Texture> createRenderTargetViewTexture() override;

  std::unique_ptr<Shadow_map_texture_pool>
  createShadowMapTexturePool(uint32_t maxDimension, uint32_t textureArraySize) override;

  void load(Texture& texture) override;
  void unload(Texture& texture) override;

  static void initStatics();
  static void destroyStatics();

 private:
  static std::string _name;
};

}// namespace oe::pipeline_d3d12
