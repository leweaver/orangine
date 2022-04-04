#pragma once

#include <OeCore/Mesh_data.h>

#include <d3d12.h>

namespace oe::pipeline_d3d12 {
class Gpu_buffer {
 public:
  /**
   * Create from the given meshBufferAccessor, and schedule the upload of the Mesh_data within.
   * When using for an index buffer, use D3D12_RESOURCE_STATE_INDEX_BUFFER
   * @param device d3d device
   * @param name Name given to the DX Object - debug builds only
   * @param meshBufferAccessor Describes the size of the buffer and contains initial data.
   */
  static std::unique_ptr<Gpu_buffer>
  create(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor, D3D12_RESOURCE_STATES bufferState = D3D12_RESOURCE_STATE_GENERIC_READ);

  /**
   * Create a buffer of the given size in bytes. Typically used for constant buffer. Does not upload data.
   * @param device d3d device
   * @param name Name given to the DX Object - debug builds only
   * @param sizeInBytes How large the buffer will be, in bytes
   */
  static std::unique_ptr<Gpu_buffer> create(ID3D12Device6* device, const std::wstring& name, size_t sizeInBytes, D3D12_RESOURCE_STATES bufferState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

  /**
   * Create a buffer of the same size as the given mesh_buffer and schedules for upload. Typically used for constant buffer
   * @param device d3d device
   * @param name Name given to the DX Object - debug builds only
   * @param sizeInBytes How large the buffer will be, in bytes
   */
  static std::unique_ptr<Gpu_buffer> create(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer& bufferData, D3D12_RESOURCE_STATES bufferState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

  D3D12_VERTEX_BUFFER_VIEW GetAsVertexBufferView() const;
  D3D12_INDEX_BUFFER_VIEW GetAsIndexBufferView(DXGI_FORMAT format) const;
  D3D12_CONSTANT_BUFFER_VIEW_DESC GetAsConstantBufferView() const;

 private:
  Gpu_buffer(
          Microsoft::WRL::ComPtr<ID3D12Resource> gpuBuffer, uint32_t gpuBufferSizeInBytes, uint32_t gpuBufferStride,
          D3D12_RESOURCE_STATES resourceState)
      : _gpuBuffer(std::move(gpuBuffer))
      , _gpuBufferSizeInBytes(gpuBufferSizeInBytes)
      , _gpuBufferStride(gpuBufferStride)
      , _resourceState(resourceState)
  {}

  const Microsoft::WRL::ComPtr<ID3D12Resource> _gpuBuffer;
  const uint32_t _gpuBufferSizeInBytes;
  const uint32_t _gpuBufferStride;
  const D3D12_RESOURCE_STATES _resourceState;
};
}// namespace oe::pipeline_d3d12