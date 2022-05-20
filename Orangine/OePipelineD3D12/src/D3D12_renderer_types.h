#pragma once

#include "Gpu_buffer.h"

#include <OeCore/Renderer_types.h>

#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <unordered_map>

namespace oe::pipeline_d3d12 {
class Descriptor_heap_pool;
struct Descriptor_range {
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  size_t incrementSize;
  uint32_t descriptorCount;
  uint32_t offsetInDescriptorsFromHeapStart;
  bool isValid() const { return cpuHandle.ptr != 0; }
};

struct Committed_gpu_resource {
  ID3D12Resource* resource = nullptr;
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
  CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
};

struct Gpu_buffer_reference {
  // CPU Source data
  std::shared_ptr<Mesh_buffer_accessor> cpuBuffer;

  // GPU Destination
  std::unique_ptr<Gpu_buffer> gpuBuffer;
};

struct Root_signature_layout
{
  inline static constexpr int32_t kSlotInvalid = -1;

  ID3D12RootSignature* rootSignature = nullptr;
  int32_t constantBufferAllVisParamIndex = kSlotInvalid;
};

}// namespace oe::pipeline_d3d12

// Must be in the regular oe namespace to match forward declarations in the non-D3D12 library.
namespace oe {
struct Mesh_gpu_data {
  unsigned int vertexCount;
  std::unordered_map<Vertex_attribute_semantic, std::shared_ptr<pipeline_d3d12::Gpu_buffer_reference>> vertexBuffers;

  // List matches the ordering of the vertex semantic layout used on this mesh
  std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;

  std::unique_ptr<pipeline_d3d12::Gpu_buffer> indexBuffer;
  DXGI_FORMAT indexFormat;
  unsigned int indexCount;

  bool failedRendering;
};
}