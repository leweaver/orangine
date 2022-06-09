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

static constexpr int64_t kSwapchainBackbufferTextureId = -1;
static constexpr int64_t kSwapchainDepthStencilTextureId = -2;

bool isSwapchainTexture(Texture_internal_id id)
{
  return id == kSwapchainBackbufferTextureId || id == kSwapchainDepthStencilTextureId;
}

std::string D3D12_texture_manager::_name;

///////////
// D3D_file_texture
class D3D12_file_texture : public D3D12_texture {
 public:
  // Filename must be an absolute path to a supported texture file.
  // If the file does not exist or is not supported, the load() method will throw.
  D3D12_file_texture(const std::string& filename, bool generateMipsIfMissing, uint32_t extraFlags = 0)
      : D3D12_texture(extraFlags)
      , _filename(filename)
      , _generateMipsIfMissing(generateMipsIfMissing)
  {}

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

  const std::string& filename() const { return _filename; }

  const std::string& getName() const override { return _filename; }

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
      : D3D12_texture({width, height}, 0, nullptr)
      , _textureDesc(createRgbDesc(width, height))
      , _name(std::move(name))
  {}

  D3D12_render_target_texture(
          const Vector2u& dimension, std::string name, D3D12_RESOURCE_DESC desc, uint32_t extraFlags = 0)
      : D3D12_texture(dimension, extraFlags, nullptr)
      , _textureDesc(desc)
      , _name(std::move(name))
  {}

  const std::string& getName() const override { return _name; }

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
    auto hr = deviceResources.GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &_textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr, IID_PPV_ARGS(&texture));

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
// D3D12_texture_swapchain_resource
class D3D12_texture_swapchain_resource : public D3D12_texture {
 public:
  D3D12_texture_swapchain_resource() = default;

  const std::string& getName() const override { return getTextureTypeName(); }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_texture_swapchain_resource";
    return textureTypeName;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    _dimension = deviceResources.getBackBufferDimensions();
    if (getInternalId() == kSwapchainBackbufferTextureId) {
      // load() is called manually by the D3D12_render_step_manager at the beginning of each frame to make sure this is
      // up to date.
      setResource(deviceResources.getCurrentFrameBackBuffer().resource, false);
    }
    else if (getInternalId() == kSwapchainDepthStencilTextureId) {
      setResource(deviceResources.getDepthStencil().resource, false);
    }
  }
};

///////////
// D3D12_depth_stencil_texture\
// TODO: Name this something more generic; and allow whatever type of format
class D3D12_depth_stencil_texture : public D3D12_texture {
 public:
  D3D12_depth_stencil_texture() = default;

  const std::string& getName() const override { return getTextureTypeName(); }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_depth_stencil_texture";
    return textureTypeName;
  }

  void load(D3D12_device_resources& deviceResources) override
  {

    OE_CHECK(deviceResources.getDepthStencil().resource);
    D3D12_RESOURCE_DESC textureDesc = deviceResources.getDepthStencil().resource->GetDesc();
    // Not actually a real depth stencil; we're using it as a shader resource. So this format won't match an actual
    // depth buffer
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    auto hr = deviceResources.GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr, IID_PPV_ARGS(&texture));

    ThrowIfFailed(hr, "Failed to creating depth stencil copy target resource");
    auto desc2 = texture->GetDesc();
    setResource(std::move(texture));
  }
};

///////////
// D3D12_shadow_map_texture_array_texture
class D3D12_shadow_map_texture_array_texture : public D3D12_texture {
 public:
  D3D12_shadow_map_texture_array_texture(Vector2u dimension, uint16_t textureArraySize)
      : D3D12_texture(dimension, static_cast<uint32_t>(Texture_flags::Is_array_texture), nullptr)
      , _textureArraySize(textureArraySize)
  {}

  const std::string& getName() const override { return getTextureTypeName(); }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_shadow_map_texture_array_texture";
    return textureTypeName;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    // Create the render target texture.

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    D3D12_RESOURCE_DESC textureDesc = createShadowMapArrayDesc(_dimension, _textureArraySize);
    auto hr = deviceResources.GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr, IID_PPV_ARGS(&texture));

    ThrowIfFailed(hr, "Failed to creating RenderTarget resource");
    setResource(std::move(texture));
  }

  static D3D12_RESOURCE_DESC createShadowMapArrayDesc(Vector2u dimension, uint16_t arraySize)
  {
    OE_CHECK(is_power_of_two(dimension.x));
    OE_CHECK(is_power_of_two(dimension.y));

    D3D12_RESOURCE_DESC textureDesc{};

    // Setup the render target texture description.
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = dimension.x;
    textureDesc.Height = dimension.y;
    textureDesc.MipLevels = 1;
    textureDesc.DepthOrArraySize = arraySize;
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;//DXGI_FORMAT_R24G8_TYPELESS;//DXGI_FORMAT_D24_UNORM_S8_UINT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    return textureDesc;
  }

 private:
  uint16_t _textureArraySize;
};

///////////
// D3D_shadow_map_texture_array_slice
class D3D12_shadow_map_texture_array_slice : public D3D12_texture {
 public:
  D3D12_shadow_map_texture_array_slice(
          std::shared_ptr<D3D12_shadow_map_texture_array_texture> arrayTexture, uint16_t arraySlice)
      : D3D12_texture(
                arrayTexture->getDimension(), static_cast<uint32_t>(Texture_flags::Is_array_slice),
                arrayTexture->getResource())
      , _arrayTexture(arrayTexture)
  {
    _arraySlice = arraySlice;
  }

  const std::string& getName() const override { return getTextureTypeName(); }

  const std::string& getTextureTypeName() const override
  {
    static std::string textureTypeName = "D3D12_shadow_map_texture_array_slice";
    return textureTypeName;
  }

  void load(D3D12_device_resources& deviceResources) override
  {
    if (!_arrayTexture->isValid()) {
      _arrayTexture->load(deviceResources);
      OE_CHECK(_arrayTexture->isValid());
    }
    setResource(_arrayTexture->getResource(), false);
  };

 private:
  std::shared_ptr<D3D12_shadow_map_texture_array_texture> _arrayTexture;
};

///////////
// D3D12_shadow_map_texture_pool
class D3D12_shadow_map_texture_pool : public Shadow_map_texture_pool {
 public:
  D3D12_shadow_map_texture_pool(
          std::shared_ptr<D3D12_shadow_map_texture_array_texture> shadowMapArrayTexture2D,
          std::vector<std::shared_ptr<D3D12_shadow_map_texture_array_slice>>&& slices)
      : Shadow_map_texture_pool()
      , _shadowMapArrayTexture2D(shadowMapArrayTexture2D)
      , _slices(std::move(slices))
  {
    for (size_t i = 0; i < _slices.size(); ++i) {
      OE_CHECK(_slices.at(i)->getArraySlice() == i);
      _availableSlices.push_back(i);
    }
  }

  std::shared_ptr<Texture> borrowTextureSlice() override
  {
    OE_CHECK_MSG(!_availableSlices.empty(), "shadow map pool is exhausted");
    auto texture = _slices.at(_availableSlices.back());
    OE_CHECK(texture);
    _availableSlices.pop_back();
    return texture;
  }

  void returnTextureSlice(std::shared_ptr<Texture> texture) override
  {
    OE_CHECK_MSG(
            _slices.at(texture->getArraySlice()) == texture,
            "Attempt to return a shadowMapTexture that doesn't belong to this pool!");
    _availableSlices.push_back(texture->getArraySlice());
  }

  std::shared_ptr<Texture> getShadowMapTextureArray() override { return _shadowMapArrayTexture2D; };

  const std::vector<std::shared_ptr<D3D12_shadow_map_texture_array_slice>>& getSlices() const { return _slices; }

 private:
  std::shared_ptr<D3D12_shadow_map_texture_array_texture> _shadowMapArrayTexture2D;
  std::vector<std::shared_ptr<D3D12_shadow_map_texture_array_slice>> _slices;
  std::vector<size_t> _availableSlices;
};


///////////
// D3D12_texture_manager
D3D12_texture_manager::D3D12_texture_manager(D3D12_device_resources& deviceResources)
    : _deviceResources(deviceResources)
{
  // Create singleton textures for backbuffer and depth/stencil textures
  _swapchainBackbufferTexture = std::make_shared<D3D12_texture_swapchain_resource>();
  _swapchainBackbufferTexture->setInternalId(kSwapchainBackbufferTextureId);
  _swapchainDepthStencilTexture = std::make_shared<D3D12_texture_swapchain_resource>();
  _swapchainDepthStencilTexture->setInternalId(kSwapchainDepthStencilTextureId);
}

void D3D12_texture_manager::initStatics() { D3D12_texture_manager::_name = "D3D12_texture_manager"; }

void D3D12_texture_manager::destroyStatics() {}

std::shared_ptr<Texture>
D3D12_texture_manager::createTextureFromBuffer(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t>& buffer)
{
  OE_THROW(std::runtime_error("Not implemented"));
}

std::shared_ptr<Texture>
D3D12_texture_manager::createTextureFromFile(const std::string& fileName, bool generateMipsIfMissing)
{
  return createTexture<D3D12_file_texture>(fileName, generateMipsIfMissing);
}

std::shared_ptr<Texture>
D3D12_texture_manager::createCubeMapTextureFromFile(const std::string& fileName, bool generateMipsIfMissing)
{
  return createTexture<D3D12_file_texture>(
          fileName, generateMipsIfMissing, static_cast<uint32_t>(Texture_flags::Is_cube_texture));
}

std::shared_ptr<Texture> D3D12_texture_manager::createRenderTargetTexture(int width, int height, std::string name)
{
  return createTexture<D3D12_render_target_texture>(width, height, name);
}

std::shared_ptr<Shadow_map_texture_pool>
D3D12_texture_manager::createShadowMapTexturePool(uint32_t maxDimension, uint32_t textureArraySize)
{
  // Create the render target texture.
  LOG(INFO) << "Creating Shadow_map_texture_pool texture2D array with ArraySize=" << textureArraySize
            << ", Width/Height=" << maxDimension;

  auto arrayTexture =
          createTexture<D3D12_shadow_map_texture_array_texture>(Vector2u{maxDimension, maxDimension}, textureArraySize);

  std::vector<std::shared_ptr<D3D12_shadow_map_texture_array_slice>> slices;
  slices.reserve(textureArraySize);
  for (uint16_t i = 0; i < textureArraySize; ++i) {
    slices.push_back(createTexture<D3D12_shadow_map_texture_array_slice>(arrayTexture, i));
  }
  auto pool = std::make_shared<D3D12_shadow_map_texture_pool>(arrayTexture, std::move(slices));

  _shadowMapTexturePools.push_back(pool);
  return pool;
}

std::shared_ptr<Texture> D3D12_texture_manager::getSwapchainBackBufferTexture() { return _swapchainBackbufferTexture; }
std::shared_ptr<Texture> D3D12_texture_manager::getSwapchainDepthStencilTexture()
{
  return _swapchainDepthStencilTexture;
}

std::shared_ptr<Texture> D3D12_texture_manager::createDepthTexture()
{
  return createTexture<D3D12_depth_stencil_texture>();
}

void D3D12_texture_manager::release(std::shared_ptr<Texture> texture)
{
  if (!texture) {
    return;
  }
  D3D12_texture* d3d12Texture = getAsTextureImpl(*texture);
  d3d12Texture->unload();
  _textures[d3d12Texture->getInternalId()] = nullptr;
}

void D3D12_texture_manager::release(std::shared_ptr<Shadow_map_texture_pool> pool)
{
  if (!pool) {
    return;
  }
  auto poolPos = std::find(_shadowMapTexturePools.begin(), _shadowMapTexturePools.end(), pool);
  OE_CHECK(poolPos != _shadowMapTexturePools.end());

  for (auto& slice : (*poolPos)->getSlices()) {
    release(slice);
  }
  release((*poolPos)->getShadowMapTextureArray());

  *poolPos = nullptr;
}

void D3D12_texture_manager::createDeviceDependentResources() {}

void D3D12_texture_manager::destroyDeviceDependentResources()
{
  for (auto& texturePtr : _textures) {
    texturePtr->unload();
  }
  _pendingLoads.clear();
}

ID3D12Resource* D3D12_texture_manager::getResource(Texture& texture) const
{
  D3D12_texture* d3d12Texture = getAsTextureImpl(texture);

  if (!d3d12Texture->isValid()) {
    d3d12Texture->load(_deviceResources);
  }
  if (!d3d12Texture->isValid()) {
    OE_THROW(std::invalid_argument("Failed to load texture"));
  }
  return d3d12Texture->getResource();
}

D3D12_texture* D3D12_texture_manager::getAsTextureImpl(const Texture& texture) const
{
  D3D12_texture* d3d12Texture{};
  if (texture.getInternalId() == kSwapchainBackbufferTextureId) {
    d3d12Texture = _swapchainBackbufferTexture.get();
  }
  else if (texture.getInternalId() == kSwapchainDepthStencilTextureId) {
    d3d12Texture = _swapchainDepthStencilTexture.get();
  }
  else {
    OE_CHECK(texture.getInternalId() >= 0 && texture.getInternalId() < _textures.size());
    d3d12Texture = _textures[texture.getInternalId()].get();
    OE_CHECK(d3d12Texture == &texture);
  }
  return d3d12Texture;
}

DXGI_FORMAT D3D12_texture_manager::getFormat(Texture& texture) const
{
  D3D12_texture* d3d12Texture = getAsTextureImpl(texture);
  return d3d12Texture->getFormat();
}

void D3D12_texture_manager::load(const Texture& texture)
{
  if (!isSwapchainTexture(texture.getInternalId())) {
    OE_CHECK(isValidTextureId(texture.getInternalId()));
    _pendingLoads.push_back(_textures[texture.getInternalId()]);
  }
}

void D3D12_texture_manager::unload(const Texture& texture)
{
  OE_CHECK(texture.getInternalId() < _textures.size());
  _textures[texture.getInternalId()]->unload();
}

void D3D12_texture_manager::updateSwapchainDependentResources()
{
  _swapchainBackbufferTexture->load(_deviceResources);
  _swapchainDepthStencilTexture->load(_deviceResources);
}

void D3D12_texture_manager::flushLoads()
{
  for (auto& pendingLoad : _pendingLoads) {
    Texture_internal_id internalId = pendingLoad->getInternalId();

    // Check that the id is still valid - the texture may have been released.
    if (isValidTextureId(internalId) && _textures.at(internalId) == pendingLoad) {
      pendingLoad->load(_deviceResources);
    }
  }
  _pendingLoads.clear();
}

bool D3D12_texture_manager::isValidTextureId(Texture_internal_id id) { return id >= 0 && id < _textures.size(); }

}// namespace oe::pipeline_d3d12