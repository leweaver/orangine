#pragma once

#include <OeCore/Render_step_manager.h>

#include "D3D12_device_resources.h"
#include "D3D12_renderer_types.h"

#include <vectormath.hpp>


namespace oe::pipeline_d3d12 {
class ITexture_manager;

class D3D12_render_step_manager final : public Render_step_manager {
 public:
  struct D3D12_render_pass_data {
    Descriptor_range renderTargets;
    /*
    std::vector<ID3D12RenderTargetView*> _renderTargetViews = {};
    Microsoft::WRL::ComPtr<ID3D12BlendState> _blendState;
    Microsoft::WRL::ComPtr<ID3D12DepthStencilState> _depthStencilState;
     */
  };
  struct D3D12_render_step_data {
    std::vector<D3D12_render_pass_data> renderPassData;
  };

  D3D12_render_step_manager(
          IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
          ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
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
  void beginRenderNamedEvent(const wchar_t* name);
  void endRenderNamedEvent();
  void createRenderStepResources();
  void destroyRenderStepResources();
  void renderSteps(const Camera_data& cameraData);

  static void initStatics();
  static void destroyStatics();

 private:
  void renderStep(Render_step& step, D3D12_render_step_data& renderStepData, const Camera_data& cameraData);

  static std::string _name;

  std::unique_ptr<D3D12_device_resources> _deviceResources;
  std::vector<D3D12_render_step_data> _renderStepData;
  std::vector<std::wstring> _passNames;
};
}// namespace oe