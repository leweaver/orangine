#include "Render_pass_copy_depth_to_resource.h"

namespace oe::pipeline_d3d12 {
Render_pass_copy_depth_to_resource::Render_pass_copy_depth_to_resource(
        D3D12_device_resources& deviceResources, D3D12_texture_manager& textureManager, CopyFromTo copyFromTo)
    : _deviceResources(deviceResources)
    , _textureManager(textureManager)
    , _preCopyBarriers(copyFromTo.size() * 2)
    , _postCopyBarriers(copyFromTo.size() * 2)
    , _copyFromTo(copyFromTo)
{
  OE_CHECK(_copyFromTo.size());
  for (size_t idx = 0; idx < _copyFromTo.size(); ++idx) {
    const auto& texture = _copyFromTo.at(idx);
    OE_CHECK(texture.first);
    OE_CHECK(texture.second);
  }
}

void Render_pass_copy_depth_to_resource::render(const Camera_data& cameraData)
{
  for (size_t idx = 0; idx < _copyFromTo.size(); ++idx) {
    const auto& texture = _copyFromTo.at(idx);
    ID3D12Resource* fromResource = _textureManager.getResource(*texture.first);
    ID3D12Resource* toResource = _textureManager.getResource(*texture.second);

    OE_CHECK(fromResource);
    OE_CHECK(toResource);

    // TODO: Support non-depth textures.
    _preCopyBarriers[idx * 2] = CD3DX12_RESOURCE_BARRIER::Transition(
            fromResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    _postCopyBarriers[idx * 2] = CD3DX12_RESOURCE_BARRIER::Transition(
            fromResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    _preCopyBarriers[idx * 2 + 1] = CD3DX12_RESOURCE_BARRIER::Transition(
            toResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    _postCopyBarriers[idx * 2 + 1] = CD3DX12_RESOURCE_BARRIER::Transition(
            toResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  }

  auto commandList = _deviceResources.GetPipelineCommandList();

  commandList->OMSetRenderTargets(0, nullptr, false, nullptr);


  commandList->ResourceBarrier(_preCopyBarriers.size(), _preCopyBarriers.data());

  for (const auto texture : _copyFromTo) {
    ID3D12Resource* fromResource = _textureManager.getResource(*texture.first);
    ID3D12Resource* toResource = _textureManager.getResource(*texture.second);

    _deviceResources.GetPipelineCommandList()->CopyResource(toResource, fromResource);
  }

  commandList->ResourceBarrier(_postCopyBarriers.size(), _postCopyBarriers.data());
}
}// namespace oe::d3d12_pipeline