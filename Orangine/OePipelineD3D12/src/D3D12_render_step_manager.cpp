#include "D3D12_render_step_manager.h"
#include "D3D12_device_resources.h"

#include <OeCore/Render_pass_generic.h>

using namespace oe;

std::string D3D12_render_step_manager::_name;// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<>
void oe::create_manager(
        Manager_instance<IRender_step_manager>& out, IScene_graph_manager& sceneGraphManager,
        IDev_tools_manager& devToolsManager, ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
        IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory, std::unique_ptr<D3D12_device_resources>& deviceResources)
{
  out = Manager_instance<IRender_step_manager>(std::make_unique<D3D12_render_step_manager>(
          sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager, lightingManager,
          primitiveMeshDataFactory, std::move(deviceResources)));
}

D3D12_render_step_manager::D3D12_render_step_manager(
        IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager, ITexture_manager& textureManager,
        IShadowmap_manager& shadowmapManager, IEntity_render_manager& entityRenderManager,
        ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        std::unique_ptr<D3D12_device_resources> deviceResources)
    : Render_step_manager(
              sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager,
              lightingManager, primitiveMeshDataFactory)
    , _deviceResources(std::move(deviceResources))
{}

// Base class overrides
void D3D12_render_step_manager::initialize()
{
  Render_step_manager::initialize();
}
void D3D12_render_step_manager::shutdown()
{
  Render_step_manager::shutdown();
}
Viewport D3D12_render_step_manager::getScreenViewport() const
{
  return {0, 0, 1, 1, 0, 1};
}

// IRender_step_manager implementation
void D3D12_render_step_manager::clearRenderTargetView(const Color& color) {}
void D3D12_render_step_manager::clearDepthStencil(float depth, uint8_t stencil) {}
std::unique_ptr<Render_pass> D3D12_render_step_manager::createShadowMapRenderPass()
{
  return std::make_unique<Render_pass_generic>([](const Camera_data&, const Render_pass&) {});
}
void D3D12_render_step_manager::beginRenderNamedEvent(const wchar_t* name) {}
void D3D12_render_step_manager::endRenderNamedEvent() {}
void D3D12_render_step_manager::createRenderStepResources() {}
void D3D12_render_step_manager::destroyRenderStepResources() {}
void D3D12_render_step_manager::renderSteps(const Camera_data& cameraData) {}

void D3D12_render_step_manager::initStatics()
{
  D3D12_render_step_manager::_name = "D3D12_render_step_manager";
}

void D3D12_render_step_manager::destroyStatics() {}
