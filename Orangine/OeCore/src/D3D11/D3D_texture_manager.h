#pragma once

#include "OeCore/ITexture_manager.h"

#include "D3D_device_repository.h"

#include "D3D_directx11.h"
#include <wrl/client.h>

namespace oe {

class D3D_texture : public Texture {
 public:
  D3D_texture() : _shaderResourceView(nullptr) {}
  D3D_texture(uint32_t extraFlags) : Texture({0, 0}, extraFlags) {
    // Make sure that the 'Is_valid' flag wasn't passed in 
    assert(!isValid());
  }

  D3D_texture(
      Uint2 dimension,
      uint32_t extraFlags,
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
      : Texture(
            dimension,
            extraFlags | static_cast<uint32_t>(
                             srv != nullptr ? Texture_flags::Is_valid : Texture_flags::Invalid))
      , _shaderResourceView(srv) {}

  void setShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) {
    if (srv == nullptr) {
      _flags = _flags & ~static_cast<uint32_t>(Texture_flags::Is_valid);
      _shaderResourceView.Reset();
    } else {
      _flags = _flags | static_cast<uint32_t>(Texture_flags::Is_valid);
      _shaderResourceView = srv;
    }

  }
  ID3D11ShaderResourceView* getShaderResourceView() const { return _shaderResourceView.Get(); }

  // Will throw a std::exception if texture failed to load.
  virtual void load(ID3D11Device* device) = 0;
  virtual void unload() { setShaderResourceView(nullptr); }

 private:
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
};

class D3D_render_target_view_texture : public D3D_texture {
 public:
  explicit D3D_render_target_view_texture(ID3D11RenderTargetView* renderTargetView)
      : _renderTargetView(renderTargetView) {}
  ID3D11RenderTargetView* renderTargetView() const { return _renderTargetView.Get(); }

  void load(ID3D11Device* device) override {}
  void unload() override {}

 protected:
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;
};

class D3D_shadow_map_texture : public D3D_texture {
 public:
  ID3D11DepthStencilView* depthStencilView() const { return _depthStencilView.Get(); }

 protected:
  D3D_shadow_map_texture(Uint2 dimension, uint32_t flags)
      : D3D_texture(dimension, flags, nullptr), _depthStencilView({}) {}
  explicit D3D_shadow_map_texture(uint32_t flags) : D3D_texture(flags), _depthStencilView({}) {}

  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthStencilView;
};

class D3D_texture_manager : public ITexture_manager {
 public:
  explicit D3D_texture_manager(
      Scene& scene,
      std::shared_ptr<D3D_device_repository> deviceRepository)
      : ITexture_manager(scene), _deviceRepository(deviceRepository) {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // ITexture_manager implementation
  std::shared_ptr<Texture> createTextureFromBuffer(
      uint32_t stride,
      uint32_t buffer_size,
      std::unique_ptr<uint8_t>& buffer) override;
  std::shared_ptr<Texture> createTextureFromFile(const std::wstring& fileName) override;
  std::shared_ptr<Texture> createTextureFromFile(
      const std::wstring& fileName,
      const Sampler_descriptor& samplerDescriptor) override;

  std::shared_ptr<Texture> createDepthTexture() override;
  std::shared_ptr<Texture> createRenderTargetTexture(int width, int height) override;
  std::shared_ptr<Texture> createRenderTargetViewTexture() override;

  std::unique_ptr<Shadow_map_texture_pool> createShadowMapTexturePool(
      uint32_t maxDimension,
      uint32_t textureArraySize) override;

  void load(Texture& texture) override;
  void unload(Texture& texture) override;

  // internal-only public members
  static D3D_texture& verifyAsD3dTexture(Texture& texture);
  static const D3D_texture& verifyAsD3dTexture(const Texture& texture);

  static D3D_render_target_view_texture& verifyAsD3dRenderTargetViewTexture(Texture& texture);
  static const D3D_render_target_view_texture& verifyAsD3dRenderTargetViewTexture(
      const Texture& texture);

  static D3D_shadow_map_texture& verifyAsD3dShadowMapTexture(Texture& texture);
  static const D3D_shadow_map_texture& verifyAsD3dShadowMapTexture(const Texture& texture);

 private:
  static std::string _name;

  std::vector<std::shared_ptr<D3D_texture>> _createdTextures;
  std::shared_ptr<D3D_device_repository> _deviceRepository;
};
} // namespace oe