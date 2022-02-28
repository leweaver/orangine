#include <d3dx12/d3dx12.h>
#include "Gpu_buffer.h"

using namespace oe::pipeline_d3d12;
using Microsoft::WRL::ComPtr;

Gpu_buffer* Gpu_buffer::create(ID3D12Device6* device, const std::wstring& name, const oe::Mesh_buffer_accessor& meshBufferAccessor)
{
  ComPtr<ID3D12Resource> buffer;

  size_t gpuBufferSize = meshBufferAccessor.count * meshBufferAccessor.stride;
  OE_CHECK(meshBufferAccessor.offset + gpuBufferSize <= meshBufferAccessor.buffer->dataSize);

  uint8_t* dataStart = meshBufferAccessor.buffer->data + meshBufferAccessor.offset;

  // TODO: using upload heaps to transfer static data like vert buffers is not recommended.
  // Every time the GPU needs it, the upload heap will be marshalled
  // over. Please read up on Default Heap usage. An upload heap is used here for
  // code simplicity and because there are very few verts to actually transfer.
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBufferSize);
  ThrowIfFailed(device->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&buffer)));

  // Copy the triangle data to the vertex buffer.
  uint8_t* pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
  ThrowIfFailed(buffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
  memcpy(pVertexDataBegin, dataStart, gpuBufferSize);
  buffer->Unmap(0, nullptr);

  if (!name.empty()) {
    buffer->SetName(name.c_str());
  }

  Gpu_buffer* gpuBuffer = new Gpu_buffer;
  gpuBuffer->gpuBuffer = buffer;
  gpuBuffer->vertexBufferView.BufferLocation = buffer->GetGPUVirtualAddress();
  gpuBuffer->vertexBufferView.StrideInBytes = meshBufferAccessor.stride;
  return gpuBuffer;
}
