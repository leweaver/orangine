#pragma once

#include <OeCore/Mesh_data.h>

#include <d3d12.h>

namespace oe::pipeline_d3d12 {
class Gpu_buffer {
 public:
  static Gpu_buffer* create(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor);

 private:
  Gpu_buffer() = default;

  Microsoft::WRL::ComPtr<ID3D12Resource> gpuBuffer;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};
}