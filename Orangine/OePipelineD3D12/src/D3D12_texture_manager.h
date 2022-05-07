#pragma once

#include "D3D12_device_resources.h"
#include "PipelineUtils.h"
#include <OeCore/ITexture_manager.h>
#include <OeCore/Texture.h>

namespace oe::pipeline_d3d12 {

class D3D12_texture : public Texture {
 public:
  D3D12_texture(uint32_t extraFlags = 0)
      : Texture(0, {0, 0}, extraFlags)
  {
    // Make sure that the 'Is_valid' flag wasn't passed in
    OE_CHECK(!isValid());
  }

  D3D12_texture(Vector2u dimension, uint32_t extraFlags, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
      : Texture(0, dimension, extraFlags)
  {
    // Make sure that the 'Is_valid' flag wasn't passed in
    OE_CHECK(!isValid());
    setResource(resource);
  }

  void setResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
  {
    if (resource == nullptr) {
      _flags = _flags & ~static_cast<uint32_t>(Texture_flags::Is_valid);
      _resource.Reset();
    }
    else {
      _flags = _flags | static_cast<uint32_t>(Texture_flags::Is_valid);
      _resource = resource;

      std::stringstream name;
      name << getTextureTypeName() << " '" << getName() << "'";
      SetObjectName(_resource.Get(), oe::utf8_decode(name.str()).c_str());
    }
  }

  virtual const std::string& getTextureTypeName() const = 0;

  void setInternalId(uint64_t id) {
    _internalId = id;
  }

  ID3D12Resource* getResource() { return _resource.Get(); }

  virtual void load(D3D12_device_resources& deviceResources) {};
  virtual void unload()
  {
    setResource(nullptr);
  }

 private:
  Microsoft::WRL::ComPtr<ID3D12Resource> _resource;
};

class D3D12_texture_manager : public ITexture_manager, public Manager_base, public Manager_deviceDependent {
 public:
  explicit D3D12_texture_manager(D3D12_device_resources& deviceResources);

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
  std::shared_ptr<Texture> createTextureFromFile(const std::string& fileName, bool generateMipsIfMissing = false) override;
  std::shared_ptr<Texture>
  createTextureFromFile(const std::string& fileName, const Sampler_descriptor& samplerDescriptor, bool generateMipsIfMissing = false) override;
  std::shared_ptr<Texture> createCubeMapTextureFromFile(const std::string& fileName, const Sampler_descriptor& samplerDescriptor, bool generateMipsIfMissing = false) override;

  std::shared_ptr<Texture> createDepthTexture() override;
  std::shared_ptr<Texture> createRenderTargetTexture(int width, int height, std::string name) override;
  std::shared_ptr<Texture> createSwapchainBackBufferTexture() override;

  std::unique_ptr<Shadow_map_texture_pool>
  createShadowMapTexturePool(uint32_t maxDimension, uint32_t textureArraySize) override;

  void load(Texture& texture) override;
  void unload(Texture& texture) override;

  static void initStatics();
  static void destroyStatics();

  ID3D12Resource* getResource(Texture& texture) const;

 private:
  template <class _Ty, class... _Types>
  std::shared_ptr<_Ty> createTexture(_Types&&... _Args) {
    auto texture = std::make_shared<_Ty>(std::forward<_Types>(_Args)...);
    texture->setInternalId(_textures.size());
    _textures.push_back(texture);
    return texture;
  }

  static std::string _name;
  std::vector<std::shared_ptr<D3D12_texture>> _textures;
  std::shared_ptr<D3D12_texture> _backbufferTexture;
  std::shared_ptr<D3D12_texture> _depthStencilTexture;

  D3D12_device_resources& _deviceResources;
  std::vector<std::shared_ptr<D3D12_texture>> _pendingLoads;
};

}// namespace oe::pipeline_d3d12
