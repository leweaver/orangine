#pragma once

#include <d3d12.h>
#include <d3dx12/d3dx12.h>

namespace oe::pipeline_d3d12 {

/**
 * The primary goal with a descriptor heap is to allocate as much memory as necessary to store all the descriptors for
 * as much rendering as possible, perhaps a frame or more.
 *
 * Performance Note: Switching descriptor heaps might—depending on the underlying hardware—result in flushing the
 * GPU pipeline. Therefore switching descriptor heaps should be minimized or paired with other operations that would
 * flush the graphics pipeline anyway. To avoid this scenario, ensure that the pool is large enough.
 */
class Descriptor_heap_pool {
 public:
  Descriptor_heap_pool();
  Descriptor_heap_pool(ID3D12Device6* device, D3D12_DESCRIPTOR_HEAP_DESC heapDesc, std::string name);

  /**
   * Tries to allocate the given number of descriptors in the current heap. If there is not enough room, a new heap is
   * created.
   * @param numDescriptors number of descriptors to allocate. Must be at most the number of descriptors supported by the
   * heap desc.
   * @return a pair of handles pointing to the beginning of a range.
   */
  Descriptor_range allocateRange(uint32_t numDescriptors);

  D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescriptorHandleForHeapStart();
  D3D12_GPU_DESCRIPTOR_HANDLE getGpuDescriptorHandleForHeapStart();

 private:

  void createNextHeap();

  ID3D12Device6* _device;
  D3D12_DESCRIPTOR_HEAP_DESC _heapDesc;
  std::string _name;
  uint32_t _handleIncrementSize;
  uint32_t _nextHeapId;

  std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> _heapsAtCapacity;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _currentHeap;
  uint32_t _currentAllocated;
};
}