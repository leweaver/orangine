#include "Gpu_buffer.h"

#include <d3dx12/d3dx12.h>

using namespace oe::pipeline_d3d12;
using Microsoft::WRL::ComPtr;

std::unique_ptr<Gpu_buffer> Gpu_buffer::createAndUpload(
        ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor,
        DirectX::ResourceUploadBatch& resourceUploadBatch, D3D12_RESOURCE_STATES bufferState)
{
  uint32_t gpuBufferSize = meshBufferAccessor.count * meshBufferAccessor.stride;
  OE_CHECK(gpuBufferSize > 0);
  OE_CHECK(meshBufferAccessor.offset + gpuBufferSize <= meshBufferAccessor.buffer->dataSize);

  auto gpuBuffer =
          Gpu_buffer::create(device, name, gpuBufferSize, meshBufferAccessor.stride, D3D12_RESOURCE_STATE_COPY_DEST);

  // Copy the source data to the resource.
  uint8_t* pCpuDataBegin = meshBufferAccessor.buffer->data + meshBufferAccessor.offset;
  gpuBuffer->uploadAndTransition(resourceUploadBatch, {pCpuDataBegin, gpuBufferSize}, bufferState);
  return gpuBuffer;
}

std::unique_ptr<Gpu_buffer> Gpu_buffer::create(
        ID3D12Device6* device, const std::wstring& name, size_t sizeInBytes, uint32_t stride,
        D3D12_RESOURCE_STATES bufferState)
{

  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
  ComPtr<ID3D12Resource> resource;
  ThrowIfFailed(device->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, bufferState, nullptr, IID_PPV_ARGS(&resource)));

  if (!name.empty()) {
    resource->SetName(name.c_str());
  }

  auto gpuBufferSize = static_cast<uint32_t>(sizeInBytes);
  return std::unique_ptr<Gpu_buffer>(new Gpu_buffer(std::move(resource), gpuBufferSize, stride, bufferState));
}

D3D12_VERTEX_BUFFER_VIEW Gpu_buffer::getAsVertexBufferView() const
{
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes, _gpuBufferStride};
}

D3D12_INDEX_BUFFER_VIEW Gpu_buffer::getAsIndexBufferView(DXGI_FORMAT format) const
{
#if !defined(NDEBUG)
  if (_resourceState != D3D12_RESOURCE_STATE_INDEX_BUFFER) {
    LOG(WARNING) << "Retrieving an indexBufferView for a Gpu_buffer with an unusual resource state: " << _resourceState;
  }
#endif
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes, format};
}

D3D12_CONSTANT_BUFFER_VIEW_DESC Gpu_buffer::getAsConstantBufferView() const
{
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes};
}


void Gpu_buffer::uploadAndTransition(
        DirectX::ResourceUploadBatch& resourceUploadBatch, const gsl::span<uint8_t>& sourceData,
        D3D12_RESOURCE_STATES toState)
{
  const auto dataSize = static_cast<UINT>(sourceData.size());
  D3D12_SUBRESOURCE_DATA subresource{sourceData.data(), dataSize, dataSize};
  ID3D12Resource* resource = _gpuBuffer.Get();
  OE_CHECK(resource);

  resourceUploadBatch.Upload(resource, 0, &subresource, 1);
  resourceUploadBatch.Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, toState);
}
