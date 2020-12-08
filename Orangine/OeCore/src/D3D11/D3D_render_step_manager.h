#pragma once

#include "OeCore/Render_step_manager.h"
#include "D3D_device_repository.h"

namespace oe {

class D3D_render_step_manager : public Render_step_manager {
public:
  D3D_render_step_manager(Scene& scene, std::shared_ptr<D3D_device_repository> device_repository);

  // Base class overrides
  void shutdown() override;
  Viewport getScreenViewport() const override;

  // IRender_step_manager implementation
  void clearRenderTargetView(const Color& color) override;
  void clearDepthStencil(float depth, uint8_t stencil) override;
  std::unique_ptr<Render_pass> createShadowMapRenderPass() override;
  void beginRenderNamedEvent(const wchar_t* name) override;
  void endRenderNamedEvent() override;
  void createRenderStepResources() override;
  void destroyRenderStepResources() override;
  void renderSteps(const Camera_data& cameraData) override;

 private:
  DX::DeviceResources& getDeviceResources() const;

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void createRenderStepResources(Render_step<TData, TRender_passes...>& step);

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void destroyRenderStepResources(Render_step<TData, TRender_passes...>& step);

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void renderStep(Render_step<TData, TRender_passes...>& step, const Camera_data& cameraData);

  template <
      Render_pass_blend_mode TBlend_mode,
      Render_pass_depth_mode TDepth_mode,
      Render_pass_stencil_mode TStencil_mode,
      uint32_t TStencil_read_mask,
      uint32_t TStencil_write_mask>
  void renderPass(
      Render_pass_config<
          TBlend_mode,
          TDepth_mode,
          TStencil_mode,
          TStencil_read_mask,
          TStencil_write_mask>& renderPassInfo,
      Render_pass& renderPass,
      const Camera_data& cameraData);

  std::shared_ptr<D3D_device_repository> _deviceRepository;
};
}