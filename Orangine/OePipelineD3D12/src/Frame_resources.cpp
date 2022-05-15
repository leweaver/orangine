#include "Frame_resources.h"

using namespace oe::pipeline_d3d12;

Frame_resources::Frame_resources(ID3D12Device4* device)
    : _device(device)
    , _currentRootSignature(nullptr)
{
  _constantBufferPools = {
          {std::make_unique<Constant_buffer_pool>(
                   device, kConstantBufferPoolSize256, kConstantBufferPoolInitialCapacity),
           std::make_unique<Constant_buffer_pool>(
                   device, kConstantBufferPoolSize512, kConstantBufferPoolInitialCapacity),
           std::make_unique<Constant_buffer_pool>(
                   device, kConstantBufferPoolSize1024, kConstantBufferPoolInitialCapacity)}};
}

Constant_buffer_pool& Frame_resources::getOrCreateConstantBufferPoolToFit(size_t itemSizeInBytes, int64_t itemCount)
{
  std::unique_ptr<Constant_buffer_pool>* bufferPtr = nullptr;

  for (auto& poolPtr : _constantBufferPools) {
    if (itemSizeInBytes <= poolPtr->getItemSizeInBytes()) {
      bufferPtr = &poolPtr;
      break;
    }
  }

  if (!bufferPtr) {
    LOG(FATAL) << "Size '" << itemSizeInBytes << "' exceeds maximum constant buffer pool item size";
  }

  if ((*bufferPtr)->getFreeCount() < itemCount) {
    // Make a new pool. Store the old one until next frame, at which point it will be deleted.
    size_t itemSize = (*bufferPtr)->getItemSizeInBytes();
    int64_t oldCapacity = (*bufferPtr)->getCapacity();
    int64_t newCapacity = std::max(itemCount, oldCapacity * 2);
    LOG(INFO) << "Constant_buffer_pool (itemSize=" << itemSize << ") is exhausted, reallocating with new size "
              << newCapacity;
    _retiredPools.push_back(std::move(*bufferPtr));
    *bufferPtr = std::make_unique<Constant_buffer_pool>(_device, itemSize, newCapacity);
  }

  return **bufferPtr;
}

void Frame_resources::reset()
{
  for (auto& poolPtr : _constantBufferPools) {
    poolPtr->reset();
  }

  if (!_retiredPools.empty()) {
    LOG(INFO) << "Retiring " << _retiredPools.size() << " constant buffer pools";
    _retiredPools.clear();
  }
}

void Frame_resources::setCurrentBoundRootSignature(ID3D12RootSignature* rootSignature)
{
  OE_CHECK(rootSignature == nullptr || !_currentRootSignature);
  _currentRootSignature = rootSignature;
}

ID3D12RootSignature* Frame_resources::getCurrentBoundRootSignature() { return _currentRootSignature; }