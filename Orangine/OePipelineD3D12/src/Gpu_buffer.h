#pragma once

#include <OeCore/Mesh_data.h>

#include <ResourceUploadBatch.h>
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
   * @param resourceUploadBatch resource uploader on which the upload will be scheduled
   * @param bufferState The state that the resource should transition to once upload is complete.
   */
  static std::unique_ptr<Gpu_buffer>
  createAndUpload(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor,
         DirectX::ResourceUploadBatch& resourceUploadBatch,
         D3D12_RESOURCE_STATES bufferState = D3D12_RESOURCE_STATE_GENERIC_READ);

  /**
   * Create a buffer of the given size in bytes. Does not upload data.
   * @param device d3d device
   * @param name Name given to the DX Object - debug builds only
   * @param sizeInBytes How large the buffer will be, in bytes
   * @param stride Only used to populate the stride in the result of @see getAsVertexBufferView
   */
  static std::unique_ptr<Gpu_buffer>
  create(ID3D12Device6* device, const std::wstring& name, size_t sizeInBytes, uint32_t stride = 0,
         D3D12_RESOURCE_STATES bufferState = D3D12_RESOURCE_STATE_COPY_DEST);

  D3D12_VERTEX_BUFFER_VIEW getAsVertexBufferView() const;
  D3D12_INDEX_BUFFER_VIEW getAsIndexBufferView(DXGI_FORMAT format) const;
  D3D12_CONSTANT_BUFFER_VIEW_DESC getAsConstantBufferView() const;

  ID3D12Resource* getResource() { return _gpuBuffer.Get(); };

  uint32_t getBufferSizeBytes() { return _gpuBufferSizeInBytes; }

  /**
   * Schedules upload of the given data, then transitions to the given state. This Gpu_buffer must already be in
   * D3D12_RESOURCE_STATE_COPY_DEST.
   * @param resourceUploadBatch resource uploader on which the upload will be scheduled
   * @param sourceData data that will be copied for upload
   * @param toState The state that the resource should transition to once upload is complete.
   */
  void uploadAndTransition(
          DirectX::ResourceUploadBatch& resourceUploadBatch, const gsl::span<uint8_t>& sourceData,
          D3D12_RESOURCE_STATES toState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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