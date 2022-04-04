#pragma once

#include "Gpu_buffer.h"

#include <OeCore/Renderer_types.h>

#include <unordered_map>
#include <d3d12.h>

namespace oe::pipeline_d3d12 {
struct Mesh_gpu_data {
  unsigned int vertexCount;
  std::unordered_map<Vertex_attribute_semantic, std::unique_ptr<Gpu_buffer>> vertexBuffers;

  std::unique_ptr<Gpu_buffer> indexBuffer;
  DXGI_FORMAT indexFormat;
  unsigned int indexCount;

  bool failedRendering;
};

struct Descriptor_range {
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  size_t incrementSize;
  uint32_t descriptorCount;
};
}