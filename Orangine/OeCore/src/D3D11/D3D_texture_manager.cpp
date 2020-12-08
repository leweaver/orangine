#include "pch.h"

#include "D3D_texture_manager.h"
#include "OeCore/Scene.h"

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include <utility>

namespace oe {
std::string D3D_texture_manager::_name = "Texture_manager";

template <>
oe::ITexture_manager* create_manager(
    Scene& scene,
    std::shared_ptr<D3D_device_repository>& deviceRepository) {
  return new D3D_texture_manager(scene, deviceRepository);
}

///////////
// D3D_buffer_texture
class D3D_buffer_texture : public D3D_texture {
 public:
  D3D_buffer_texture(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t>& buffer)
      : _stride(stride), _bufferSize(buffer_size), _buffer(std::move(buffer)) {}

  void load(ID3D11Device* device) override { OE_THROW(std::runtime_error("Not implemented")); }
  void unload() override { setShaderResourceView(nullptr); }

 private:
  uint32_t _stride;
  uint32_t _bufferSize;
  std::unique_ptr<uint8_t> _buffer;
};

///////////
// D3D_file_texture
class D3D_file_texture : public D3D_texture {
  Microsoft::WRL::ComPtr<ID3D11Resource> _textureResource;
  std::wstring _filename;

 public:
  // Filename must be an absolute path to a supported texture file.
  // If the file does not exist or is not supported, the load() method will throw.
  explicit D3D_file_texture(const std::wstring& filename) : _filename(filename) {
    _samplerDescriptor = Sampler_descriptor(Default_values());
  }
  D3D_file_texture(const std::wstring& filename, const Sampler_descriptor& samplerDescriptor)
      : _filename(filename) {
    _samplerDescriptor = samplerDescriptor;
  }

  void load(ID3D11Device* device) override {

    unload();

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    if (str_ends(_filename, L".dds"))
      hr = DirectX::CreateDDSTextureFromFile(
          device,
          _filename.c_str(),
          _textureResource.ReleaseAndGetAddressOf(),
          srv.ReleaseAndGetAddressOf());
    else
      hr = DirectX::CreateWICTextureFromFile(
          device,
          _filename.c_str(),
          _textureResource.ReleaseAndGetAddressOf(),
          srv.ReleaseAndGetAddressOf());

    setShaderResourceView(srv);

    if (FAILED(hr)) {
      LOG(WARNING) << "Failed to load " << utf8_encode(_filename);
      return;
    }
  }

  void unload() override {
    D3D_texture::unload();
    _textureResource.Reset();
  }

  const std::wstring& filename() const { return _filename; }
};

///////////
// D3D_depth_texture
class D3D_depth_texture : public D3D_texture {
 public:
  explicit D3D_depth_texture(const DX::DeviceResources& deviceResources)
      : _deviceResources(deviceResources) {}

  void load(ID3D11Device* device) override {
    D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
    sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sr_desc.Texture2D.MostDetailedMip = 0;
    sr_desc.Texture2D.MipLevels = -1;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    ThrowIfFailed(
        device->CreateShaderResourceView(
            _deviceResources.GetDepthStencil(), &sr_desc, srv.ReleaseAndGetAddressOf()),
        "DepthTexture shaderResourceView");
    setShaderResourceView(srv);
  }
  void unload() override { D3D_texture::unload(); }

 private:
  const DX::DeviceResources& _deviceResources;
};

///////////
// D3D_render_target_texture
class D3D_render_target_texture : public D3D_render_target_view_texture {
 public:
  static D3D_render_target_texture* createDefaultRgb(uint32_t width, uint32_t height) {
    // Initialize the render target texture description.
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    // Setup the render target texture description.
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    return new D3D_render_target_texture(textureDesc);
  }

  void load(ID3D11Device* d3dDevice) override {
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

    // Create the render target texture.
    ThrowIfFailed(
        d3dDevice->CreateTexture2D(
            &_textureDesc, nullptr, _renderTargetTexture.ReleaseAndGetAddressOf()),
        "Creating RenderTarget texture2D");

    // Setup the description of the render target view.
    renderTargetViewDesc.Format = _textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    ThrowIfFailed(
        d3dDevice->CreateRenderTargetView(
            _renderTargetTexture.Get(),
            &renderTargetViewDesc,
            _renderTargetView.ReleaseAndGetAddressOf()),
        "Creating RenderTarget renderTargetView");

    // Setup the description of the shader resource view.
    shaderResourceViewDesc.Format = _textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    ThrowIfFailed(
        d3dDevice->CreateShaderResourceView(
            _renderTargetTexture.Get(), &shaderResourceViewDesc, srv.ReleaseAndGetAddressOf()),
        "Creating RenderTarget shaderResourceView");
    setShaderResourceView(srv);
  }
  void unload() override {
    D3D_texture::unload();

    _renderTargetTexture.Reset();
    _renderTargetView.Reset();
  }

 private:
  explicit D3D_render_target_texture(D3D11_TEXTURE2D_DESC textureDesc)
      : D3D_render_target_view_texture(nullptr)
      , _textureDesc(textureDesc)
      , _renderTargetTexture(nullptr) {}

  D3D11_TEXTURE2D_DESC _textureDesc;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> _renderTargetTexture;
};

///////////
// D3D_shadow_map_texture_array_texture
class D3D_shadow_map_texture_array_texture : public D3D_texture {
 public:
  D3D_shadow_map_texture_array_texture(
      Uint2 dimension,
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
      : D3D_texture(
            dimension,
            static_cast<uint32_t>(Texture_flags::Is_array_texture),
            std::move(srv)) {}

  void load(ID3D11Device*) override {}
  void unload() override {}
};

///////////
// D3D_shadow_map_texture_basic
class D3D_shadow_map_texture_basic : public D3D_shadow_map_texture {
 public:
  D3D_shadow_map_texture_basic(uint32_t width, uint32_t height)
      : D3D_shadow_map_texture({width, height}, 0) {}

  void load(ID3D11Device* device) override {
    // Initialize the render target texture description.
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    // Setup the render target texture description.
    textureDesc.Width = _dimension.x;
    textureDesc.Height = _dimension.y;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowMapTexture;
    ThrowIfFailed(
        device->CreateTexture2D(&textureDesc, nullptr, &shadowMapTexture),
        "Creating Shadow_map_texture texture2D");

    // depth stencil resource view desc.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    depthStencilViewDesc.Flags = 0;
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    ThrowIfFailed(
        device->CreateDepthStencilView(
            shadowMapTexture.Get(), &depthStencilViewDesc, &_depthStencilView),
        "Creating Shadow_map_texture depthStencilView");

    // shader resource view desc.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

    // Create the shader resource view.
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    ThrowIfFailed(
        device->CreateShaderResourceView(
            shadowMapTexture.Get(), &shaderResourceViewDesc, srv.ReleaseAndGetAddressOf()),
        "Creating Shadow_map_texture shaderResourceView");
    setShaderResourceView(srv);
  }

  void unload() override {
    _depthStencilView.Reset();
    D3D_texture::unload();
  }
};

///////////
// D3D_shadow_map_texture_array_slice
class D3D_shadow_map_texture_array_slice : public D3D_shadow_map_texture {
 public:
  struct Array_texture {
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* srv;
    uint32_t arraySize;
  };

  typedef std::function<Array_texture()> Array_texture_source;

  D3D_shadow_map_texture_array_slice(
      uint32_t arraySlice,
      uint32_t textureWidth,
      Array_texture_source arrayTextureSource)
      : D3D_shadow_map_texture(
            {textureWidth, textureWidth},
            static_cast<uint32_t>(Texture_flags::Is_array_slice))
      , _arrayTextureSource(arrayTextureSource) {
    _arraySlice = arraySlice;
  }

  void load(ID3D11Device* device) override {
    // Create the render target texture.
    Array_texture shadowMapTexture = _arrayTextureSource();

    // depth stencil resource view desc.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    depthStencilViewDesc.Flags = 0;
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = shadowMapTexture.arraySize == 1
                                             ? D3D11_DSV_DIMENSION_TEXTURE2D
                                             : D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    depthStencilViewDesc.Texture2DArray.ArraySize = 1;
    depthStencilViewDesc.Texture2DArray.MipSlice = 0;
    depthStencilViewDesc.Texture2DArray.FirstArraySlice = _arraySlice;

    ThrowIfFailed(
        device->CreateDepthStencilView(
            shadowMapTexture.texture, &depthStencilViewDesc, &_depthStencilView),
        "Creating Shadow_map_texture depthStencilView");

    // Should use a shader resource view from the array texture, not the slice. Make sure that the
    // caller assigned one.
    setShaderResourceView(shadowMapTexture.srv);
  }

  void unload() override {
    _depthStencilView.Reset();
    D3D_texture::unload();
  }

 private:
  Array_texture_source _arrayTextureSource;
};

///////////
// D3D_shadow_map_texture_pool
class D3D_shadow_map_texture_pool : public Shadow_map_texture_pool {
 public:
  D3D_shadow_map_texture_pool(
      uint32_t maxDimension,
      uint32_t textureArraySize,
      std::shared_ptr<D3D_device_repository> deviceRepository)
      : _dimension(maxDimension)
      , _textureArraySize(textureArraySize)
      , _deviceRepository(deviceRepository) {
    assert(is_power_of_two(maxDimension));
  }

  ~D3D_shadow_map_texture_pool() {
    if (_shadowMapArrayTexture2D) {
      LOG(WARNING) << "D3D_shadow_map_texture_pool: Should call destroyDeviceDependentResources "
                      "prior "
                      "to object deletion";

      unloadTextures();
    }
  }

  void createDeviceDependentResources() override {
    auto device = _deviceRepository->deviceResources().GetD3DDevice();

    // Initialize the render target texture description.
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    // Setup the render target texture description.
    textureDesc.Width = _dimension;
    textureDesc.Height = _dimension;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = _textureArraySize;
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    LOG(INFO) << "Creating Shadow_map_texture_pool texture2D array with ArraySize="
              << _textureArraySize << ", Width/Height=" << _dimension;

    ThrowIfFailed(
        device->CreateTexture2D(&textureDesc, nullptr, &_shadowMapArrayTexture2D),
        "Creating Shadow_map_texture_pool texture2D array");

    // Shader resource view for the array texture, which is shared by all of the slices.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shaderResourceViewDesc.ViewDimension =
        _textureArraySize == 1 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    shaderResourceViewDesc.Texture2DArray.ArraySize = _textureArraySize;
    shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
    shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthShaderResourceView;
    ThrowIfFailed(
        device->CreateShaderResourceView(
            _shadowMapArrayTexture2D.Get(), &shaderResourceViewDesc, &depthShaderResourceView),
        "Creating Shadow_map_texture_pool depth shaderResourceView");

    shaderResourceViewDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stencilShaderResourceView;
    ThrowIfFailed(
        device->CreateShaderResourceView(
            _shadowMapArrayTexture2D.Get(), &shaderResourceViewDesc, &stencilShaderResourceView),
        "Creating Shadow_map_texture_pool stencil shaderResourceView");

    // Create the shadow maps
    _shadowMapDepthArrayTexture = std::make_shared<D3D_shadow_map_texture_array_texture>(
        Uint2{_dimension, _dimension}, depthShaderResourceView);
    _shadowMapStencilArrayTexture = std::make_shared<D3D_shadow_map_texture_array_texture>(
        Uint2{_dimension, _dimension}, stencilShaderResourceView);

    auto arrayTextureRetriever = [this, depthShaderResourceView]() {
      return D3D_shadow_map_texture_array_slice::Array_texture{
          _shadowMapArrayTexture2D.Get(), depthShaderResourceView.Get(), _textureArraySize};
    };

    for (uint32_t slice = 0; slice < _textureArraySize; ++slice) {
      auto shadowMap = std::make_unique<D3D_shadow_map_texture_array_slice>(
          _textureArraySize - slice - 1, _dimension, arrayTextureRetriever);
      _shadowMaps.push_back(move(shadowMap));
    }
  }

  void destroyDeviceDependentResources() override {
    if (_shadowMaps.size() != _textureArraySize) {
      OE_THROW(std::logic_error(
          "Must return all borrowed textures prior to a call to destroyDeviceDependentResources"));
    }

    unloadTextures();
  }

  void unloadTextures() {
    _shadowMaps.resize(0);

    _shadowMapDepthArrayTexture->unload();
    _shadowMapDepthArrayTexture.reset();
    _shadowMapStencilArrayTexture->unload();
    _shadowMapStencilArrayTexture.reset();
    _shadowMapArrayTexture2D.Reset();
  }

  // If the pool is waiting for textures to be returned, this will throw.
  // If the pool is exhausted (all textures are allocated) then this will return nullptr.
  std::shared_ptr<Texture> borrowTexture() override {
    if (_shadowMaps.empty())
      OE_THROW(std::runtime_error("shadow map pool is exhausted"));

    auto shadowMap = move(_shadowMaps.back());
    _shadowMaps.pop_back();

    return shadowMap;
  }

  // once a texture has been returned to the pool, no more can be borrowed until
  // all have been returned back to the pool.
  void returnTexture(std::shared_ptr<Texture> texture) override {
    assert(texture != nullptr);
    auto& shadowMap = D3D_texture_manager::verifyAsD3dShadowMapTexture(*texture);

    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    shadowMap.getShaderResourceView()->GetResource(&resource);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
    ThrowIfFailed(resource.As<ID3D11Texture2D>(&texture2d));

    if (texture2d.Get() != _shadowMapArrayTexture2D.Get())
      OE_THROW(std::logic_error(
          "Attempt to return a shadowMapTexture that doesn't belong to this pool!"));

    _shadowMaps.push_back(texture);
  }

  // Shader resource view that can be used when sampling the shadow map depth
  std::shared_ptr<Texture> shadowMapDepthTextureArray() override {
    return _shadowMapDepthArrayTexture;
  };

  // Shader resource view that can be used when sampling the shadow map stencil
  std::shared_ptr<Texture> shadowMapStencilTextureArray() override {
    return _shadowMapStencilArrayTexture;
  };

 protected:
  const uint32_t _dimension;
  const uint32_t _textureArraySize;

  std::shared_ptr<D3D_device_repository> _deviceRepository;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> _shadowMapArrayTexture2D;
  std::shared_ptr<D3D_texture> _shadowMapDepthArrayTexture;
  std::shared_ptr<D3D_texture> _shadowMapStencilArrayTexture;

  std::vector<std::shared_ptr<Texture>> _shadowMaps;
};

///////////
// D3D_texture_manager
void D3D_texture_manager::createDeviceDependentResources() {}
void D3D_texture_manager::destroyDeviceDependentResources() {
  for (auto tex : _createdTextures) {
    if (tex->isValid()) {
      tex->unload();
    }
  }
}

std::shared_ptr<Texture> D3D_texture_manager::createTextureFromBuffer(
    uint32_t stride,
    uint32_t buffer_size,
    std::unique_ptr<uint8_t>& buffer) {
  auto texture = std::make_shared<D3D_buffer_texture>(stride, buffer_size, buffer);
  _createdTextures.push_back(texture);
  return texture;
}

std::shared_ptr<Texture> D3D_texture_manager::createTextureFromFile(const std::wstring& fileName) {
  auto texture = std::make_shared<D3D_file_texture>(fileName);
  _createdTextures.push_back(texture);
  return texture;
}

std::shared_ptr<Texture> D3D_texture_manager::createTextureFromFile(
    const std::wstring& fileName,
    const Sampler_descriptor& samplerDescriptor) {
  auto texture = std::make_shared<D3D_file_texture>(fileName, samplerDescriptor);
  _createdTextures.push_back(texture);
  return texture;
}

std::shared_ptr<Texture> D3D_texture_manager::createDepthTexture() {
  auto texture = std::make_shared<D3D_depth_texture>(_deviceRepository->deviceResources());
  _createdTextures.push_back(texture);
  return texture;
}

std::shared_ptr<Texture> D3D_texture_manager::createRenderTargetTexture(int width, int height) {
  auto texture =
      std::shared_ptr<D3D_texture>(D3D_render_target_texture::createDefaultRgb(width, height));
  _createdTextures.push_back(texture);
  return texture;
}

std::shared_ptr<Texture> D3D_texture_manager::createRenderTargetViewTexture() {
  auto texture = std::make_shared<D3D_render_target_view_texture>(
      _deviceRepository->deviceResources().GetRenderTargetView());
  _createdTextures.push_back(texture);
  return texture;
}

std::unique_ptr<Shadow_map_texture_pool> D3D_texture_manager::createShadowMapTexturePool(
    uint32_t maxDimension,
    uint32_t textureArraySize) {
  return std::make_unique<D3D_shadow_map_texture_pool>(
      maxDimension, textureArraySize, _deviceRepository);
}

D3D_texture& D3D_texture_manager::verifyAsD3dTexture(Texture& texture) {
  auto* cast = dynamic_cast<D3D_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}

const D3D_texture& D3D_texture_manager::verifyAsD3dTexture(const Texture& texture) {
  const auto* const cast = dynamic_cast<const D3D_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}

D3D_render_target_view_texture& D3D_texture_manager::verifyAsD3dRenderTargetViewTexture(
    Texture& texture) {
  auto* cast = dynamic_cast<D3D_render_target_view_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}
const D3D_render_target_view_texture& D3D_texture_manager::verifyAsD3dRenderTargetViewTexture(
    const Texture& texture) {
  const auto* const cast = dynamic_cast<const D3D_render_target_view_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}

D3D_shadow_map_texture& D3D_texture_manager::verifyAsD3dShadowMapTexture(Texture& texture) {
  auto* cast = dynamic_cast<D3D_shadow_map_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}

const D3D_shadow_map_texture& D3D_texture_manager::verifyAsD3dShadowMapTexture(
    const Texture& texture) {
  const auto* const cast = dynamic_cast<const D3D_shadow_map_texture*>(&texture);
  assert(cast != nullptr);
  return *cast;
}

void D3D_texture_manager::load(Texture& texture) {
  auto& d3dTexture = verifyAsD3dTexture(texture);
  d3dTexture.load(_deviceRepository->deviceResources().GetD3DDevice());
}

void D3D_texture_manager::unload(Texture& texture) {
  auto& d3dTexture = verifyAsD3dTexture(texture);
  d3dTexture.unload();
}

} // namespace oe
