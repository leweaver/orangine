#pragma once

#include <OeCore/EngineUtils.h>
#include <d3dx12/d3dx12.h>

namespace oe::pipeline_d3d12 {

class Constant_buffer_pool {
 public:
  inline static constexpr size_t kConstantBufferAlignment = 256;

  Constant_buffer_pool(ID3D12Device* device, size_t itemSizeInBytes, int64_t capacityInItems);

  inline int64_t getCapacity() const noexcept { return _capacityInItems; }

  inline int64_t getUsedCount() const noexcept { return _allocatedItems; }

  inline int64_t getFreeCount() const noexcept { return _capacityInItems - _allocatedItems; }

  inline size_t getItemSizeInBytes() const noexcept { return _itemSizeInBytes; }

  bool isExhausted() noexcept { return _capacityInItems == _allocatedItems; }

  /**
   * @return false if the heap is exhausted
   */
  bool getTop(uint8_t*& outItemCpuAddress, D3D12_GPU_VIRTUAL_ADDRESS& outItemGpuAddress) noexcept;

  /**
   * Attempting to pop more items than are present will result in a fatal termination
   */
  void pop(int32_t itemCount);

  void reset();

  ID3D12Resource* getUploadHeap() const { return _uploadHeap.Get(); }

 private:
  const int64_t _capacityInItems;
  const size_t _itemSizeInBytes;
  int64_t _allocatedItems;

  Microsoft::WRL::ComPtr<ID3D12Resource> _uploadHeap;
  uint8_t* _bufferItems;
};

}// namespace oe::pipeline_d3d12
