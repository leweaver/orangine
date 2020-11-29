#pragma once

#include "OeCore/Renderer_types.h"

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
}
