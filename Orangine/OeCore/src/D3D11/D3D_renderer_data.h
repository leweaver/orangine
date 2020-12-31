#pragma once

#include "OeCore/Renderer_types.h"
#include "OeCore/Renderer_data.h"
#include "OeCore/Render_light_data.h"


#include <d3d11.h>

namespace oe {

struct D3D_buffer {
  D3D_buffer(ID3D11Buffer* buffer, size_t size);
  D3D_buffer();
  ~D3D_buffer();

  ID3D11Buffer* const d3dBuffer;
  const size_t size;
};

struct D3D_buffer_accessor {
  D3D_buffer_accessor(std::shared_ptr<D3D_buffer> buffer, uint32_t stride, uint32_t offset);
  ~D3D_buffer_accessor();

  std::shared_ptr<D3D_buffer> buffer;
  uint32_t stride;
  uint32_t offset;
};

struct Renderer_data {
  Renderer_data();
  ~Renderer_data();

  void release();

  unsigned int vertexCount;
  std::map<Vertex_attribute_semantic, std::unique_ptr<D3D_buffer_accessor>> vertexBuffers;

  std::unique_ptr<D3D_buffer_accessor> indexBufferAccessor;
  DXGI_FORMAT indexFormat;
  unsigned int indexCount;

  D3D11_PRIMITIVE_TOPOLOGY topology;

  bool failedRendering;
};

template <uint8_t TMax_lights>
class D3D_render_light_data : public Render_light_data_impl<TMax_lights> {
 public:
  explicit D3D_render_light_data(ID3D11Device* device) {
    // DirectX constant buffers must be aligned to 16 byte boundaries (vector4)
    static_assert(sizeof(Light_constants) % 16 == 0);

    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(Light_constants);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = &_lightConstants;
    initData.SysMemPitch = 0;
    initData.SysMemSlicePitch = 0;

    DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &_constantBuffer));

    DirectX::SetDebugObjectName(_constantBuffer.Get(), L"Render_light_data constant buffer");
  }

  ID3D11Buffer* buffer() const { return _constantBuffer.Get(); }

  void updateBuffer(ID3D11DeviceContext* context) {
    context->UpdateSubresource(_constantBuffer.Get(), 0, nullptr, &_lightConstants, 0, 0);
  }

 protected:
  Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
};

using D3D_renderer_data = Renderer_data;
}
