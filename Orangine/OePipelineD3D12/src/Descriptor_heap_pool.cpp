#include "Descriptor_heap_pool.h"

#include <OeCore/EngineUtils.h>

#include "PipelineUtils.h"

using namespace oe::pipeline_d3d12;

Descriptor_heap_pool::Descriptor_heap_pool(ID3D12Device6* device, D3D12_DESCRIPTOR_HEAP_DESC heapDesc, std::string name)
    : _device(device)
    , _heapDesc(heapDesc)
    , _name(name)
    , _nextHeapId(0)
{
  OE_CHECK(_device);
  OE_CHECK(heapDesc.NumDescriptors > 0);

  _handleIncrementSize = _device->GetDescriptorHandleIncrementSize(heapDesc.Type);

  createNextHeap();
}

Descriptor_heap_pool::Descriptor_heap_pool()
    : _device(nullptr)
    , _heapDesc()
    , _name(nullptr)
    , _handleIncrementSize(0)
    , _nextHeapId(0)
    , _currentAllocated(0)
{
  _heapDesc.NumDescriptors = 0;
}

void Descriptor_heap_pool::createNextHeap()
{
  if (_currentHeap) {
    _heapsAtCapacity.push_back(_currentHeap);
    _currentHeap.Reset();
  }

  DX::ThrowIfFailed(_device->CreateDescriptorHeap(&_heapDesc, IID_PPV_ARGS(&_currentHeap)));

  SetObjectNameIndexed(_currentHeap.Get(), _name.c_str(), _nextHeapId);
  ++_nextHeapId;
  _currentAllocated = 0;
}

Descriptor_heap_pool::Descriptor_range Descriptor_heap_pool::allocateRange(uint32_t numDescriptors)
{
  if (_currentAllocated + numDescriptors > _heapDesc.NumDescriptors) {
    OE_CHECK(_heapDesc.NumDescriptors >= numDescriptors);
    createNextHeap();
  }

  int32_t offsetScaledByIncrementSize = _currentAllocated * _handleIncrementSize;
  _currentAllocated += numDescriptors;

  return {CD3DX12_CPU_DESCRIPTOR_HANDLE(
                  _currentHeap->GetCPUDescriptorHandleForHeapStart(), offsetScaledByIncrementSize),
          CD3DX12_GPU_DESCRIPTOR_HANDLE(
                  _currentHeap->GetGPUDescriptorHandleForHeapStart(), offsetScaledByIncrementSize),
          _handleIncrementSize, numDescriptors};
}

D3D12_CPU_DESCRIPTOR_HANDLE Descriptor_heap_pool::getCpuDescriptorHandleForHeapStart()
{
  return _currentHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE Descriptor_heap_pool::getGpuDescriptorHandleForHeapStart()
{
  return _currentHeap->GetGPUDescriptorHandleForHeapStart();
}
