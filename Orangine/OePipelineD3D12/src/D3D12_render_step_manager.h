#pragma once

#include <OeCore/Render_step_manager.h>

#include "D3D12_renderer_data.h"
#include "D3D12_device_resources.h"

#include <vectormath.hpp>

namespace oe {
class ITexture_manager;

class D3D12_render_step_manager final : public Render_step_manager {
 public:
  D3D12_render_step_manager(
          IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
          ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
          IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory, std::unique_ptr<D3D12_device_resources> deviceResources);

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
  static std::string _name;

  std::unique_ptr<D3D12_device_resources> _deviceResources;
};
}