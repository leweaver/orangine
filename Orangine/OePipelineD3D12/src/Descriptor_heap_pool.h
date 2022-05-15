#pragma once

#include "D3D12_renderer_types.h"

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
  inline static constexpr size_t kMaxDescriptorRangeSize = 16;

  Descriptor_heap_pool();
  Descriptor_heap_pool(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC heapDesc, size_t frameCount, std::string name);

  void initialize();
  void destroy();

  bool isShaderVisible() const;

  /**
   * Tries to allocate the given number of descriptors in the current heap. If the heap doesn't have sufficient room,
   * a fatal error is thrown.
   * @note - currently only supports single descriptors
   * @param numDescriptors number of descriptors to allocate. Must be at most the number of descriptors supported by the
   * heap desc.
   * @return a pair of handles pointing to the beginning of a range.
   */
  Descriptor_range allocateRange(uint32_t numDescriptors);
  uint32_t getAvailableDescriptorCount() const;

  void releaseRange(Descriptor_range&);
  void releaseRangeAtFrameStart(Descriptor_range&, size_t frameIndex);

  D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescriptorHandleForHeapStart();
  D3D12_GPU_DESCRIPTOR_HANDLE getGpuDescriptorHandleForHeapStart();

  ID3D12DescriptorHeap* getDescriptorHeap() { return _currentHeap.Get(); }

  void handleFrameStart(size_t frameIndex);

 private:

  ID3D12Device* _device;
  D3D12_DESCRIPTOR_HEAP_DESC _heapDesc;
  std::string _name;
  size_t _handleIncrementSize = 0;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _currentHeap;

  // Index of position of first descriptor in the range from the heap start
  std::array<std::vector<uint32_t>, kMaxDescriptorRangeSize> _freeRanges;
  std::vector<std::vector<Descriptor_range>> _releaseAtFrameStart;
  uint32_t _currentAllocated = 0;
};
}// namespace oe::pipeline_d3d12