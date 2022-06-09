#pragma once

#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"
#include <OeCore/IEntity_render_manager.h>

namespace oe::pipeline_d3d12 {

class D3D12_render_pass_shadow  : public Render_pass {
 public:
  D3D12_render_pass_shadow(
          D3D12_device_resources& deviceRepository,
          IScene_graph_manager& sceneGraphManager,
          IShadowmap_manager& shadowMapManager,
          D3D12_texture_manager& textureManager,
          IEntity_render_manager& entityRenderManager);

  void render(const Camera_data& cameraData) override;

 private:
  const CD3DX12_CPU_DESCRIPTOR_HANDLE& getResourceForSlice(Texture& texture);
  void renderShadowMapArray(ID3D12GraphicsCommandList* commandList);

  D3D12_device_resources& _deviceResources;
  IScene_graph_manager& _sceneGraphManager;
  IShadowmap_manager& _shadowMapManager;
  D3D12_texture_manager& _textureManager;
  IEntity_render_manager& _entityRenderManager;

  std::shared_ptr<Entity_filter> _renderableEntities;
  std::shared_ptr<Entity_filter> _lightEntities;

  // Allocated DSV descriptor handles - one entry for each array slice. Each range will be size=1
  std::vector<Descriptor_range> _shadowmapSliceResources;
};
}