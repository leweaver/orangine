//
// Created by Lewis weaver on 5/18/2022.
//

#pragma once

#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"

#include <d3d12.h>

namespace oe::pipeline_d3d12 {

class Render_pass_copy_depth_to_resource : public Render_pass {
 public:
  using CopyFromTo = std::vector<std::pair<std::shared_ptr<Texture>, std::shared_ptr<Texture>>>;
  Render_pass_copy_depth_to_resource(
          D3D12_device_resources& deviceResources, D3D12_texture_manager& textureManager, CopyFromTo copyFromTo);

  void render(const Camera_data& cameraData) override;

 private:
  D3D12_device_resources& _deviceResources;
  D3D12_texture_manager& _textureManager;
  std::vector<CD3DX12_RESOURCE_BARRIER> _preCopyBarriers;
  std::vector<CD3DX12_RESOURCE_BARRIER> _postCopyBarriers;
  CopyFromTo _copyFromTo;
};
}// namespace oe::pipeline_d3d12
