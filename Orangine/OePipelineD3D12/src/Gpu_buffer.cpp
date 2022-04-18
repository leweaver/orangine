#include "Gpu_buffer.h"

#include <d3dx12/d3dx12.h>

using namespace oe::pipeline_d3d12;
using Microsoft::WRL::ComPtr;

std::unique_ptr<Gpu_buffer>
Gpu_buffer::create(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor, D3D12_RESOURCE_STATES bufferState)
{
  ComPtr<ID3D12Resource> buffer;

  uint32_t gpuBufferSize = meshBufferAccessor.count * meshBufferAccessor.stride;
  OE_CHECK(meshBufferAccessor.offset + gpuBufferSize <= meshBufferAccessor.buffer->dataSize);

  uint8_t* pCpuDataBegin = meshBufferAccessor.buffer->data + meshBufferAccessor.offset;

  // TODO: using upload heaps to transfer static data like vert buffers is not recommended.

  // D3D12 ERROR: ID3D12Device::CreateCommittedResource: Certain resources are restricted to certain D3D12_RESOURCE_STATES states, and cannot be changed.
  // Resources on D3D12_HEAP_TYPE_UPLOAD heaps requires D3D12_RESOURCE_STATE_GENERIC_READ.
  // Reserved buffers used exclusively for texture placement requires D3D12_RESOURCE_STATE_COMMON. [ RESOURCE_MANIPULATION ERROR #741: RESOURCE_BARRIER_INVALID_HEAP]
  bufferState |= D3D12_RESOURCE_STATE_GENERIC_READ;
  // Every time the GPU needs it, the upload heap will be marshalled
  // over. Please read up on Default Heap usage. An upload heap is used here for
  // code simplicity and because there are very few verts to actually transfer.
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBufferSize);
  ThrowIfFailed(device->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, bufferState, nullptr,
          IID_PPV_ARGS(&buffer)));

  // Copy the source data to the buffer.
  uint8_t* pGpuDataBegin;
  CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
  ThrowIfFailed(buffer->Map(0, &readRange, reinterpret_cast<void**>(&pGpuDataBegin)));
  memcpy(pGpuDataBegin, pCpuDataBegin, gpuBufferSize);
  buffer->Unmap(0, nullptr);

  if (!name.empty()) {
    buffer->SetName(name.c_str());
  }

  return std::unique_ptr<Gpu_buffer>(
          new Gpu_buffer(std::move(buffer), gpuBufferSize, meshBufferAccessor.stride, bufferState));
}

std::unique_ptr<Gpu_buffer> Gpu_buffer::create(ID3D12Device6* device, const std::wstring& name, size_t sizeInBytes, D3D12_RESOURCE_STATES bufferState)
{
  ComPtr<ID3D12Resource> buffer;

  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
  ThrowIfFailed(device->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, bufferState, nullptr,
          IID_PPV_ARGS(&buffer)));

  auto gpuBufferSize = static_cast<uint32_t>(sizeInBytes);
  return std::unique_ptr<Gpu_buffer>(new Gpu_buffer(std::move(buffer), gpuBufferSize, gpuBufferSize, bufferState));
}

D3D12_VERTEX_BUFFER_VIEW Gpu_buffer::GetAsVertexBufferView() const
{
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes, _gpuBufferStride};
}

D3D12_INDEX_BUFFER_VIEW Gpu_buffer::GetAsIndexBufferView(DXGI_FORMAT format) const
{
#if !defined(NDEBUG)
  if (_resourceState != D3D12_RESOURCE_STATE_INDEX_BUFFER) {
    LOG(WARNING) << "Retrieving an indexBufferView for a Gpu_buffer with an unusual resource state: " << _resourceState;
  }
#endif
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes, format};
}

D3D12_CONSTANT_BUFFER_VIEW_DESC Gpu_buffer::GetAsConstantBufferView() const
{
  return {_gpuBuffer->GetGPUVirtualAddress(), _gpuBufferSizeInBytes};
}
