#pragma once

#include "Constant_buffer_pool.h"
#include "Descriptor_heap_pool.h"
#include <OeCore/EngineUtils.h>
#include <d3dx12/d3dx12.h>

namespace oe::pipeline_d3d12 {

/**
 * Contains various buffers that are used to generate the frame; and will be recycled by another frame in the future.
 */
class Frame_resources {
 public:
  inline static constexpr size_t kConstantBufferPoolSize256 = 256;
  inline static constexpr size_t kConstantBufferPoolSize512 = 512;
  inline static constexpr size_t kConstantBufferPoolSize1024 = 1024;
  inline static constexpr size_t kConstantBufferPoolInitialCapacity = 16;

  // Any RootSignature format that uses the constant buffer below will store the descriptor table at the following
  // root signature index.
  inline static constexpr size_t kConstantBufferRootSignatureIndex = 0;

  Frame_resources(ID3D12Device4* device);

  Constant_buffer_pool& getOrCreateConstantBufferPoolToFit(size_t itemSizeInBytes, int64_t itemCount);

  // Used to share the current root signature that is bound by the material manager
  void setCurrentBoundRootSignature(ID3D12RootSignature* rootSignature);

  ID3D12RootSignature* getCurrentBoundRootSignature();

  void reset();

 private:

  std::array<std::unique_ptr<Constant_buffer_pool>, 3> _constantBufferPools;
  std::vector<std::unique_ptr<Constant_buffer_pool>> _retiredPools;

  ID3D12Device4* _device;
  ID3D12RootSignature* _currentRootSignature;
};

}// namespace oe::pipeline_d3d12