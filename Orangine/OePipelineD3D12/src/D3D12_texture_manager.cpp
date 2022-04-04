//
// Created by Lewis weaver on 3/20/2022.
//

#include "D3D12_texture_manager.h"
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <WICTextureLoader.h>

namespace oe::pipeline_d3d12 {

void D3D12_texture_manager::initStatics()
{
  D3D12_texture_manager::_name = "D3D12_texture_manager";
}

void D3D12_texture_manager::destroyStatics() {}
}// namespace oe::pipeline_d3d12

namespace oe::pipeline_d3d12 {

///////////
// D3D_file_texture
class D3D12_file_texture : public D3D12_texture {
 public:
  // Filename must be an absolute path to a supported texture file.
  // If the file does not exist or is not supported, the load() method will throw.
  D3D12_file_texture(const std::string& filename, bool generateMipsIfMissing)
      : _filename(filename)
      , _generateMipsIfMissing(generateMipsIfMissing)
  {}

  void load(oe::D3D12_device_resources& deviceResources) override
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

 private:
  std::string _filename;
  bool _generateMipsIfMissing;
};

///////////
// D3D12_render_target_texture
class D3D12_render_target_texture : public D3D12_texture {
 public:
  static D3D12_render_target_texture* createDefaultRgb(uint32_t width, uint32_t height) {
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

    return new D3D12_render_target_texture(desc);
  }

  void load(oe::D3D12_device_resources& deviceResources) override {
    // Create the render target texture.

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    auto hr = deviceResources.GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &_textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&texture));

    ThrowIfFailed(hr, "Creating RenderTarget texture2D");
  }

 private:
  explicit D3D12_render_target_texture(D3D12_RESOURCE_DESC textureDesc)
      : D3D12_texture({0, 0}, 0, nullptr)
      , _textureDesc(textureDesc) {}

  D3D12_RESOURCE_DESC _textureDesc;
};


}// namespace oe::pipeline_d3d12