#include "Constant_buffer_pool.h"

using oe::pipeline_d3d12::Constant_buffer_pool;

Constant_buffer_pool::Constant_buffer_pool(ID3D12Device* device, size_t itemSizeInBytes, int64_t capacityInItems)
    : _capacityInItems(capacityInItems)
    , _allocatedItems(0)
    , _itemSizeInBytes(itemSizeInBytes)
{
  OE_CHECK(itemSizeInBytes % kConstantBufferAlignment == 0);

  // Create an upload heap for the constant buffers.
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(itemSizeInBytes * capacityInItems);
  ThrowIfFailed(device->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&_uploadHeap)));

  CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
  ThrowIfFailed(_uploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&_bufferItems)));

  reset();
}

bool Constant_buffer_pool::getTop(
        uint8_t*& outItemCpuAddress, D3D12_GPU_VIRTUAL_ADDRESS& outItemGpuAddress) noexcept
{
  if (isExhausted()) {
    return false;
  }

  size_t offset = _allocatedItems * _itemSizeInBytes;
  outItemCpuAddress = _bufferItems + offset;
  outItemGpuAddress = _uploadHeap->GetGPUVirtualAddress() + offset;
  return true;
}

void Constant_buffer_pool::pop(int32_t itemCount)
{
  if (itemCount > getFreeCount() || itemCount < 1) {
    LOG(FATAL) << "Invalid item count '" << itemCount
               << "': must be greater than zero and at most the free count of the pool (" << getFreeCount() << ")";
  }

  _allocatedItems += itemCount;
}

void Constant_buffer_pool::reset()
{
  _allocatedItems = 0;
}
