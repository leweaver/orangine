#include "Descriptor_heap_pool.h"

#include <OeCore/EngineUtils.h>

#include "PipelineUtils.h"

using namespace oe::pipeline_d3d12;

Descriptor_heap_pool::Descriptor_heap_pool(ID3D12Device6* device, D3D12_DESCRIPTOR_HEAP_DESC heapDesc, std::string name)
    : _device(device)
    , _heapDesc(heapDesc)
    , _name(name)
{
  OE_CHECK(_device);
  OE_CHECK(heapDesc.NumDescriptors > 0);

  _handleIncrementSize = _device->GetDescriptorHandleIncrementSize(heapDesc.Type);

  createHeap();
}

Descriptor_heap_pool::Descriptor_heap_pool()
    : _device(nullptr)
    , _heapDesc()
    , _handleIncrementSize(0)
    , _currentAllocated(0)
{
  _heapDesc.NumDescriptors = 0;
}

void Descriptor_heap_pool::createHeap()
{
  OE_CHECK(!_currentHeap);

  ThrowIfFailed(_device->CreateDescriptorHeap(&_heapDesc, IID_PPV_ARGS(&_currentHeap)));

  SetObjectName(_currentHeap.Get(), _name.c_str());
  _currentAllocated = 0;
}

Descriptor_range Descriptor_heap_pool::allocateRange(uint32_t numDescriptors)
{
  OE_CHECK(numDescriptors <= getAvailableDescriptorCount());

  auto offsetScaledByIncrementSize = static_cast<int32_t>(_currentAllocated * _handleIncrementSize);
  uint32_t offsetInDescriptorsFromHeapStart = _currentAllocated;
  _currentAllocated += numDescriptors;

  return {CD3DX12_CPU_DESCRIPTOR_HANDLE(
                  _currentHeap->GetCPUDescriptorHandleForHeapStart(), offsetScaledByIncrementSize),
          CD3DX12_GPU_DESCRIPTOR_HANDLE(
                  _currentHeap->GetGPUDescriptorHandleForHeapStart(), offsetScaledByIncrementSize),
          _handleIncrementSize, numDescriptors, offsetInDescriptorsFromHeapStart};
}

uint32_t Descriptor_heap_pool::getAvailableDescriptorCount() const
{
  return _heapDesc.NumDescriptors - _currentAllocated;
}

D3D12_CPU_DESCRIPTOR_HANDLE Descriptor_heap_pool::getCpuDescriptorHandleForHeapStart()
{
  return _currentHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_heap_pool::getGpuDescriptorHandleForHeapStart()
{
  return _currentHeap->GetGPUDescriptorHandleForHeapStart();
}
void Descriptor_heap_pool::releaseRange(const Descriptor_range&)
{
  // TODO: return to some kind of pool?
}
