#include "Descriptor_heap_pool.h"

#include <OeCore/EngineUtils.h>

#include "PipelineUtils.h"

using namespace oe::pipeline_d3d12;

Descriptor_heap_pool::Descriptor_heap_pool(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC heapDesc, size_t frameCount, std::string name)
    : _device(device)
    , _heapDesc(heapDesc)
    , _name(name)
{
  OE_CHECK(_device);

  _handleIncrementSize = _device->GetDescriptorHandleIncrementSize(heapDesc.Type);
  _releaseAtFrameStart.resize(frameCount);
}

Descriptor_heap_pool::Descriptor_heap_pool()
    : _device(nullptr)
    , _heapDesc()
    , _handleIncrementSize(0)
    , _currentAllocated(0)
{
  _heapDesc.NumDescriptors = 0;
}

bool Descriptor_heap_pool::isShaderVisible() const
{
  return (_heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0;
}

void Descriptor_heap_pool::initialize()
{
  OE_CHECK(!_currentHeap);
  OE_CHECK(_heapDesc.NumDescriptors > 0);

  if (isShaderVisible()) {
    OE_CHECK(_heapDesc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && _heapDesc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    OE_CHECK(_heapDesc.NumDescriptors <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
    if (_heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
      OE_CHECK(_heapDesc.NumDescriptors <= D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);
    }
  }

  ThrowIfFailed(_device->CreateDescriptorHeap(&_heapDesc, IID_PPV_ARGS(&_currentHeap)));

  SetObjectName(_currentHeap.Get(), _name.c_str());

  _currentAllocated = 0;
  for (auto& rangeStarts : _freeRanges) {
    rangeStarts.clear();
  }
  for (auto& ranges : _releaseAtFrameStart) {
    ranges.clear();
  }
}

void Descriptor_heap_pool::destroy() {
  _currentHeap.Reset();
}

Descriptor_range Descriptor_heap_pool::allocateRange(uint32_t numDescriptors)
{
  auto& freeRangesForSize = _freeRanges.at(numDescriptors);
  uint32_t offsetInDescriptorsFromHeapStart{};
  if (freeRangesForSize.empty()) {
    // No previously freed descriptors available, create new.
    OE_CHECK(getAvailableDescriptorCount() >= numDescriptors);
    offsetInDescriptorsFromHeapStart = _currentAllocated;
    _currentAllocated += numDescriptors;
  }
  else {
    offsetInDescriptorsFromHeapStart = freeRangesForSize.back();
    freeRangesForSize.pop_back();
  }

  auto offsetScaledByIncrementSize = static_cast<int32_t>(offsetInDescriptorsFromHeapStart * _handleIncrementSize);

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
  OE_CHECK(isShaderVisible());
  return _currentHeap->GetGPUDescriptorHandleForHeapStart();
}

void Descriptor_heap_pool::releaseRange(Descriptor_range& range)
{
  OE_CHECK(range.cpuHandle.ptr != 0);
  OE_CHECK(range.descriptorCount != 0);
  _freeRanges.at(range.descriptorCount).push_back(range.offsetInDescriptorsFromHeapStart);
  range = {};
}

void Descriptor_heap_pool::releaseRangeAtFrameStart(Descriptor_range& range, size_t frameIndex)
{
  _releaseAtFrameStart[frameIndex].push_back(range);
  range = {};
}
void Descriptor_heap_pool::handleFrameStart(size_t frameIndex) {
  for (auto& range : _releaseAtFrameStart.at(frameIndex)) {
    _freeRanges.at(range.descriptorCount).push_back(range.offsetInDescriptorsFromHeapStart);
  }
}
