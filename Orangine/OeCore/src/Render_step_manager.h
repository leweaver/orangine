#pragma once

#include <OeCore/IRender_step_manager.h>
#include <OeCore/Light_provider.h>
#include <OeCore/Render_pass.h>
#include <OeCore/Render_pass_config.h>
#include <OeCore/Renderable.h>

#include <memory>

namespace oe::internal {
class Render_step_manager : public IRender_step_manager {
 public:
  explicit Render_step_manager(Scene& scene);

  // Manager_base implementations
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
  void destroyDeviceDependentResources() override;

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window,
                                          int width, int height) override;
  void destroyWindowSizeDependentResources() override;

  // IRender_step_manager implementation
  void createRenderSteps() override;
  Render_pass::Camera_data createCameraData(Camera_component& component) const override;
  void render(std::shared_ptr<Entity> cameraEntity) override;

 private:
  Render_pass::Camera_data createCameraData(const SSE::Matrix4& worldTransform, float fov,
                                            float nearPlane, float farPlane) const;

  DX::DeviceResources& deviceResources() const;
  void clearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) const;

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void createRenderStepResources(Render_step<TData, TRender_passes...>& step);

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void destroyRenderStepResources(Render_step<TData, TRender_passes...>& step);

  template <int TRender_pass_idx = 0, class TData, class... TRender_passes>
  void renderStep(Render_step<TData, TRender_passes...>& step,
                  const Render_pass::Camera_data& cameraData);

  template <Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode,
            Render_pass_stencil_mode TStencil_mode, uint32_t TStencil_read_mask,
            uint32_t TStencil_write_mask>
  void renderPass(Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask,
                                     TStencil_write_mask>& renderPassInfo,
                  Render_pass& renderPass, const Render_pass::Camera_data& cameraData);

  template <Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode,
            Render_pass_stencil_mode TStencil_mode, uint32_t TStencil_read_mask,
            uint32_t TStencil_write_mask>
  void
  renderEntity(Entity* entity, const Render_pass::Camera_data& cameraData,
               Light_provider::Callback_type& lightProvider,
               const Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask,
                                        TStencil_write_mask>& renderPassInfo);

  void renderLights(const Render_pass::Camera_data& cameraData, Render_pass_blend_mode blendMode);

  // RenderStep definitions
  struct Render_step_deferred_data {
    std::shared_ptr<Deferred_light_material> deferredLightMaterial;
    Renderable pass0ScreenSpaceQuad;
    Renderable pass2ScreenSpaceQuad;
  };
  struct Render_step_empty_data {
  };

  static std::string _name;

  Render_step<Render_step_empty_data,
              Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite,
                                 Render_pass_stencil_mode::Enabled, 0xFF, 0xFF>>
      _renderStep_shadowMap;

  Render_step<
      Render_step_deferred_data,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled>,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>,
      Render_pass_config<Render_pass_blend_mode::Additive, Render_pass_depth_mode::Disabled>>
      _renderStep_entityDeferred;

  Render_step<Render_step_empty_data, Render_pass_config<Render_pass_blend_mode::Blended_Alpha,
                                                         Render_pass_depth_mode::ReadWrite>>
      _renderStep_entityStandard;

  Render_step<Render_step_empty_data,
              Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::WriteOnly>>
      _renderStep_debugElements;

  Render_step<Render_step_empty_data,
              Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>>
      _renderStep_skybox;

  // Broad rendering
  std::unique_ptr<Entity_alpha_sorter> _alphaSorter;
  std::unique_ptr<Entity_cull_sorter> _cullSorter;

  // Entities
  std::shared_ptr<Entity_filter> _renderableEntities;
  std::shared_ptr<Entity_filter> _lightEntities;

  Light_provider::Callback_type _simpleLightProvider;

  bool _fatalError;
  bool _enableDeferredRendering;

  std::array<double, 5> _renderTimes = {};
  uint32_t _renderCount = 0;
};
} // namespace oe::internal
