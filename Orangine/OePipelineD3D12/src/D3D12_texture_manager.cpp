//
// Created by Lewis weaver on 3/20/2022.
//

#include "D3D12_texture_manager.h"
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <WICTextureLoader.h>

template<>
void oe::create_manager(
        Manager_instance<ITexture_manager>& out, oe::pipeline_d3d12::D3D12_device_resources& deviceResources)
{
  out = Manager_instance<ITexture_manager>(
          std::make_unique<oe::pipeline_d3d12::D3D12_texture_manager>(deviceResources));
}

namespace oe::pipeline_d3d12 {

static constexpr int64_t kBackbufferTextureId = -1;
static constexpr int64_t kDepthStencilTextureId = -2;

std::string D3D12_texture_manager::_name;

///////////
// D3D_file_texture
class D3D12_file_texture : public D3D12_texture {
 public:
  // Filename must be an absolute path to a supported texture file.
  // If the file does not exist or is not supported, the load() method will throw.
  D3D12_file_texture(const std::string& filename, bool generateMipsIfMissing, Sampler_descriptor samplerDescriptor, uint32_t extraFlags = 0)
      : D3D12_texture(extraFlags)
      , _filename(filename)
      , _generateMipsIfMissing(generateMipsIfMissing)
  {
    _samplerDescriptor = samplerDescriptor;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    unload();

    HRESULT hr{};
    std::wstring filenameWide = oe::utf8_decode(_filename);
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    if (oe::str_ends(_filename, ".dds")) {
      hr = DirectX::CreateDDSTextureFromFile(
              deviceResources.GetD3DDevice(), deviceResources.getResourceUploader(), filenameWide.c_str(),
              textureResource.ReleaseAndGetAddressOf(), _generateMipsIfMissing);
    }
    else {
      hr = DirectX::CreateWICTextureFromFile(
              deviceResources.GetD3DDevice(), deviceResources.getResourceUploader(), filenameWide.c_str(),
              textureResource.ReleaseAndGetAddressOf(), _generateMipsIfMissing);
    }

    setResource(textureResource);

    if (FAILED(hr)) {
      LOG(WARNING) << "Failed to load " << _filename;
      return;
    }
  }

  const std::string& filename() const
  {
    return _filename;
  }

  const std::string& getName() const override
  {
    return _filename;
  }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_file_texture";
    return textureTypeName;
  }

 private:
  std::string _filename;
  bool _generateMipsIfMissing;
};

///////////
// D3D12_render_target_texture
class D3D12_render_target_texture : public D3D12_texture {
 public:
  D3D12_render_target_texture(uint32_t width, uint32_t height, std::string name)
      : _textureDesc(createRgbDesc(width, height))
      , _name(std::move(name))
  {}

  const std::string& getName() const override
  {
    return _name;
  }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_render_target_texture";
    return textureTypeName;
  }

  D3D12_RESOURCE_DESC createRgbDesc(uint32_t width, uint32_t height)
  {
    // Initialize the render target texture description.
    D3D12_RESOURCE_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.DepthOrArraySize = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    return desc;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    // Create the render target texture.

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    // WAS COPY_DEST
    auto hr = deviceResources.GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &_textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
            IID_PPV_ARGS(&texture));

    auto format = texture->GetDesc().Format;

    ThrowIfFailed(hr, "Failed to creating RenderTarget resource");
    setResource(std::move(texture));
  }

 private:
  explicit D3D12_render_target_texture(D3D12_RESOURCE_DESC textureDesc)
      : D3D12_texture({0, 0}, 0, nullptr)
      , _textureDesc(textureDesc)
  {}

  std::string _name;
  D3D12_RESOURCE_DESC _textureDesc;
};

///////////
// D3D12_texture_backbuffer
class D3D12_texture_backbuffer : public D3D12_texture {
 public:
  D3D12_texture_backbuffer(Vector2u dimensions)
      : D3D12_texture(std::move(dimensions), 0, nullptr)
  {}

  const std::string& getName() const override
  {
    return getTextureTypeName();
  }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_texture_backbuffer";
    return textureTypeName;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    setResource(deviceResources.getCurrentFrameBackBuffer().resource);
  }
};

///////////
// D3D12_texture_manager
class D3D12_depth_stencil_texture : public D3D12_texture
{
 public:
  D3D12_depth_stencil_texture(Vector2u dimension, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
      : D3D12_texture(dimension, 0, resource)
  {}

  const std::string& getName() const override
  {
    return getTextureTypeName();
  }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_depth_stencil_texture";
    return textureTypeName;
  }
};

///////////
// D3D12_texture_manager
D3D12_texture_manager::D3D12_texture_manager(D3D12_device_resources& deviceResources)
    : _deviceResources(deviceResources)
{
  // Create singleton textures for backbuffer and depth/stencil textures
  _backbufferTexture = std::make_shared<D3D12_texture_backbuffer>(_deviceResources.getBackBufferDimensions());
  _backbufferTexture->setInternalId(kBackbufferTextureId);
  _depthStencilTexture = std::make_shared<D3D12_depth_stencil_texture>(
          _deviceResources.getBackBufferDimensions(), _deviceResources.getDepthStencil().resource);
  _depthStencilTexture->setInternalId(kDepthStencilTextureId);
}

void D3D12_texture_manager::initStatics()
{
  D3D12_texture_manager::_name = "D3D12_texture_manager";
}

void D3D12_texture_manager::destroyStatics() {}

std::shared_ptr<Texture>
D3D12_texture_manager::createTextureFromBuffer(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t>& buffer)
{
  OE_THROW(std::runtime_error("Not implemented"));
}

std::shared_ptr<Texture> D3D12_texture_manager::createTextureFromFile(const std::string& fileName, bool generateMipsIfMissing)
{
  return createTexture<D3D12_file_texture>(fileName, generateMipsIfMissing, Sampler_descriptor(Default_values()));
}

std::shared_ptr<Texture>
D3D12_texture_manager::createTextureFromFile(const std::string& fileName, const Sampler_descriptor& samplerDescriptor, bool generateMipsIfMissing)
{
  return createTexture<D3D12_file_texture>(fileName, generateMipsIfMissing, samplerDescriptor);
}

std::shared_ptr<Texture>
D3D12_texture_manager::createCubeMapTextureFromFile(const std::string& fileName, const Sampler_descriptor& samplerDescriptor, bool generateMipsIfMissing)
{
  return createTexture<D3D12_file_texture>(fileName, generateMipsIfMissing, samplerDescriptor, static_cast<uint32_t>(Texture_flags::Is_cube_texture));
}

std::shared_ptr<Texture> D3D12_texture_manager::createRenderTargetTexture(int width, int height, std::string name)
{
  return createTexture<D3D12_render_target_texture>(width, height, name);
}

std::shared_ptr<Texture> D3D12_texture_manager::createSwapchainBackBufferTexture()
{
  return _backbufferTexture;
}

std::shared_ptr<Texture> D3D12_texture_manager::createDepthTexture()
{
  return _depthStencilTexture;
}

void D3D12_texture_manager::createDeviceDependentResources() {}

void D3D12_texture_manager::destroyDeviceDependentResources()
{
  for (auto& texturePtr : _textures) {
    texturePtr->unload();
  }
}

ID3D12Resource* D3D12_texture_manager::getResource(Texture& texture) const
{
  D3D12_texture* d3d12Texture = nullptr;
  if (texture.internalId() == kBackbufferTextureId) {
    d3d12Texture = _backbufferTexture.get();
  }
  else if (texture.internalId() == kDepthStencilTextureId) {
    d3d12Texture = _depthStencilTexture.get();
  }
  else {
    OE_CHECK(texture.internalId() >= 0 && texture.internalId() < _textures.size());
    d3d12Texture = _textures[texture.internalId()].get();
  }

  if (!d3d12Texture->isValid()) {
    d3d12Texture->load(_deviceResources);
  }
  if (!d3d12Texture->isValid()) {
    OE_THROW(std::invalid_argument("Failed to load texture"));
  }
  return d3d12Texture->getResource();
}

void D3D12_texture_manager::load(Texture& texture)
{
  OE_CHECK(texture.internalId() < _textures.size());
  _pendingLoads.push_back(_textures[texture.internalId()]);
}

void D3D12_texture_manager::unload(Texture& texture)
{
  OE_CHECK(texture.internalId() < _textures.size());
  _textures[texture.internalId()]->unload();
}

std::unique_ptr<Shadow_map_texture_pool>
D3D12_texture_manager::createShadowMapTexturePool(uint32_t maxDimension, uint32_t textureArraySize)
{
  OE_CHECK(false && "Not implemented");
  return std::unique_ptr<Shadow_map_texture_pool>();
}

}// namespace oe::pipeline_d3d12