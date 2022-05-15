#pragma once

#include <OeCore/Render_step_manager.h>

#include "D3D12_device_resources.h"
#include "D3D12_renderer_types.h"
#include "D3D12_texture_manager.h"

#include <vectormath.hpp>


namespace oe {
class ITexture_manager;
}

namespace oe::pipeline_d3d12 {

class D3D12_render_step_manager final : public Render_step_manager {
 public:
  struct D3D12_render_pass_data {
    std::vector<std::pair<TextureInternalId, Descriptor_range>> renderTargetDescriptors;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuHandles;
  };
  struct D3D12_render_step_data {
    std::vector<D3D12_render_pass_data> renderPassData;
  };

  D3D12_render_step_manager(
          IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
          D3D12_texture_manager& textureManager, IShadowmap_manager& shadowmapManager,
          IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager,
          IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
          std::unique_ptr<D3D12_device_resources> deviceResources);

  // Base class overrides
  Viewport getScreenViewport() const override;

  // Manager_base implementations
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override
  {
    return _name;
  }

  // IRender_step_manager implementation
  void clearRenderTargetView(const Color& color);
  void clearDepthStencil(float f, uint8_t a);
  std::unique_ptr<Render_pass> createShadowMapRenderPass();
  std::unique_ptr<Render_pass> createResourceUploadBeginPass() override;
  std::unique_ptr<Render_pass> createResourceUploadEndPass() override;
  void beginRenderNamedEvent(const wchar_t* name) override;
  void endRenderNamedEvent() override;
  void createRenderStepResources();
  void destroyRenderStepResources();
  void renderSteps(const Camera_data& cameraData);

  static void initStatics();
  static void destroyStatics();

 private:
  void transitionToRenderTargets(const std::vector<std::shared_ptr<Texture>>& textures);
  void renderStep(Render_step& step, D3D12_render_step_data& renderStepData, const Camera_data& cameraData);

  static std::string _name;

  std::unique_ptr<D3D12_device_resources> _deviceResources;
  D3D12_texture_manager& _textureManagerD3D12;

  std::vector<D3D12_render_step_data> _renderStepData;
  std::vector<std::wstring> _passNames;

  std::unordered_map<int32_t, std::pair<Descriptor_range, size_t>> _textureIdToDescriptorRangesAndUsageCount;
  std::vector<std::shared_ptr<Texture>> _activeCustomRenderTargets;
};
}// namespace oe