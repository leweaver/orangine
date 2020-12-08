#pragma once

#include <OeCore/Entity_sorter.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/Light_provider.h>
#include <OeCore/Render_pass.h>
#include <OeCore/Render_pass_config.h>
#include <OeCore/Renderable.h>

#include <memory>

namespace oe {
class Color;

class Render_step_manager : public IRender_step_manager {
 public:
  // Manager_base implementations
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(HWND window, int width, int height) override;
  void destroyWindowSizeDependentResources() override;

  // IRender_step_manager implementation
  void createRenderSteps() override;
  Camera_data createCameraData(Camera_component& component) const override;
  void render(std::shared_ptr<Entity> cameraEntity) override;

  static constexpr size_t maxRenderTargetViews() { return 3; }

 protected:
  Render_step_manager(Scene& scene);

  // pure virtual method interface
  virtual void clearRenderTargetView(const Color& color) = 0;
  virtual void clearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) = 0;
  virtual std::unique_ptr<Render_pass> createShadowMapRenderPass() = 0;
  virtual void beginRenderNamedEvent(const wchar_t* name) = 0;
  virtual void endRenderNamedEvent() = 0;
  virtual void createRenderStepResources() = 0;
  virtual void destroyRenderStepResources() = 0;
  virtual void renderSteps(const Camera_data& cameraData) = 0;

 protected:
  Camera_data createCameraData(
      const SSE::Matrix4& worldTransform,
      float fov,
      float nearPlane,
      float farPlane) const;

  template <
      Render_pass_blend_mode TBlend_mode,
      Render_pass_depth_mode TDepth_mode,
      Render_pass_stencil_mode TStencil_mode,
      uint32_t TStencil_read_mask,
      uint32_t TStencil_write_mask>
  void renderEntity(
      Entity* entity,
      const Camera_data& cameraData,
      Light_provider::Callback_type& lightProvider,
      const Render_pass_config<
          TBlend_mode,
          TDepth_mode,
          TStencil_mode,
          TStencil_read_mask,
          TStencil_write_mask>& renderPassInfo);

  void renderLights(const Camera_data& cameraData, Render_pass_blend_mode blendMode);

  // RenderStep definitions
  struct Render_step_deferred_data {
    std::shared_ptr<Deferred_light_material> deferredLightMaterial;
    Renderable pass0ScreenSpaceQuad;
    Renderable pass2ScreenSpaceQuad;
  };
  struct Render_step_empty_data {};

  static std::string _name;

  Render_step<
      Render_step_empty_data,
      Render_pass_config<
          Render_pass_blend_mode::Opaque,
          Render_pass_depth_mode::Read_write,
          Render_pass_stencil_mode::Enabled,
          0xFF,
          0xFF>>
      _renderStep_shadowMap;

  Render_step<
      Render_step_deferred_data,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled>,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Read_write>,
      Render_pass_config<Render_pass_blend_mode::Additive, Render_pass_depth_mode::Disabled>>
      _renderStep_entityDeferred;

  Render_step<
      Render_step_empty_data,
      Render_pass_config<Render_pass_blend_mode::Blended_alpha, Render_pass_depth_mode::Read_write>>
      _renderStep_entityStandard;

  Render_step<
      Render_step_empty_data,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Write_only>>
      _renderStep_debugElements;

  Render_step<
      Render_step_empty_data,
      Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Read_write>>
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
} // namespace oe
