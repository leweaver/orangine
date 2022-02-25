#pragma once

#include <OeCore/Entity_sorter.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/IDev_tools_manager.h>
#include <OeCore/ITexture_manager.h>
#include <OeCore/IShadowmap_manager.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/Light_provider.h>
#include <OeCore/Render_pass.h>
#include <OeCore/Renderable.h>

#include <memory>

namespace oe {
class Color;

class Render_step_manager : public IRender_step_manager, public Manager_base, public Manager_deviceDependent, public Manager_windowDependent {
 public:
  // Manager_base implementations
  void initialize() override;
  void shutdown() override;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(HWND window, int width, int height) override;
  void destroyWindowSizeDependentResources() override;

  // IRender_step_manager implementation
  void createRenderSteps() override;
  Camera_data createCameraData(Camera_component& component) const override;
  void render() override;
  BoundingFrustumRH createFrustum(const Camera_component& cameraComponent) override;
  void setCameraEntity(std::shared_ptr<Entity> cameraEntity) override;

  static constexpr size_t maxRenderTargetViews() { return 3; }

 protected:
  Render_step_manager(
          IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
          ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
          IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager);

  // pure virtual method interface
  virtual void clearRenderTargetView(const Color& color) = 0;
  virtual void clearDepthStencil(float depth, uint8_t stencil) = 0;
  virtual std::unique_ptr<Render_pass> createShadowMapRenderPass() = 0;
  virtual void beginRenderNamedEvent(const wchar_t* name) = 0;
  virtual void endRenderNamedEvent() = 0;
  virtual void createRenderStepResources() = 0;
  virtual void destroyRenderStepResources() = 0;
  virtual void renderSteps(const Camera_data& cameraData) = 0;

  Camera_data createCameraData(
      const SSE::Matrix4& worldTransform,
      float fov,
      float nearPlane,
      float farPlane) const;

  void renderEntity(
      Entity* entity,
      const Camera_data& cameraData,
      Light_provider::Callback_type& lightProvider,
      const Depth_stencil_config& depthStencilConfig) const;

  void renderLights(const Camera_data& cameraData, Render_pass_blend_mode blendMode);

  void applyEnvironmentVolume(const Vector3& cameraPos);

  // RenderStep definitions
  struct Render_step_deferred_data {
    std::shared_ptr<Deferred_light_material> deferredLightMaterial;
    Renderable pass0ScreenSpaceQuad;
    Renderable pass2ScreenSpaceQuad;
  };

  struct Render_step {
    explicit Render_step(std::wstring name) : name(std::move(name)) {}
    Render_step(std::unique_ptr<Render_pass>&& renderPass, std::wstring name);
    std::wstring name;
    bool enabled = true;
    std::vector<std::unique_ptr<Render_pass>> renderPasses;
  };

  std::vector<std::unique_ptr<Render_step>> _renderSteps;
  Render_step_deferred_data _renderPassDeferredData;

  // Broad rendering
  std::unique_ptr<Entity_alpha_sorter> _alphaSorter;
  std::unique_ptr<Entity_cull_sorter> _cullSorter;

  // Entities
  std::shared_ptr<Entity_filter> _renderableEntities;
  std::shared_ptr<Entity_filter> _lightEntities;
  std::shared_ptr<Entity> _cameraEntity;

  Light_provider::Callback_type _simpleLightProvider;

  bool _fatalError;
  bool _enableDeferredRendering;

  std::array<double, 5> _renderTimes = {};
  uint32_t _renderCount = 0;

  IScene_graph_manager& _sceneGraphManager;
  IDev_tools_manager& _devToolsManager;
  ITexture_manager& _textureManager;
  IShadowmap_manager& _shadowmapManager;
  IEntity_render_manager& _entityRenderManager;
  ILighting_manager& _lightingManager;
};
} // namespace oe
