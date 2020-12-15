#include "pch.h"

#include "D3D_render_pass_shadow.h"

#include "OeCore/Entity.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/IShadowmap_manager.h"
#include "OeCore/Light_component.h"
#include "OeCore/Math_constants.h"
#include "OeCore/Mesh_utils.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Scene.h"

#include "D3D_device_repository.h"
#include "D3D_texture_manager.h"

using namespace DirectX;
using namespace oe;

Render_pass_shadow::Render_pass_shadow(
    Scene& scene,
    std::shared_ptr<D3D_device_repository> device_repository,
    size_t maxRenderTargetViews)
    : _scene(scene), _deviceRepository(device_repository) {
  _renderTargetViews.resize(maxRenderTargetViews, nullptr);

  _renderableEntities =
      _scene.manager<IScene_graph_manager>().getEntityFilter({Renderable_component::type()});
  _lightEntities = _scene.manager<IScene_graph_manager>().getEntityFilter(
      {Directional_light_component::type(),
       Point_light_component::type(),
       Ambient_light_component::type()},
      Entity_filter_mode::Any);
}

void Render_pass_shadow::render(const Camera_data&) {
  auto& d3DDeviceResources = _deviceRepository->deviceResources();

  // Render shadow maps for each shadow enabled light
  for (const auto& lightEntity : *_lightEntities) {
    // Directional light only, right now
    auto* const component = lightEntity->getFirstComponentOfType<Directional_light_component>();
    if (component && component->shadowsEnabled()) {

      Shadow_map_data* shadowData = component->shadowData().get();

      // If this is the first time rendering, initialize a new shadowmap.
      if (!shadowData) {
        component->shadowData() = std::make_unique<Shadow_map_data>();
        shadowData = component->shadowData().get();
        shadowData->shadowMap = _scene.manager<IShadowmap_manager>().borrowTexture();
      }

      // Iterate over the shadow *receivers* and build an orthographic frustum from that.

      // Project the receiver bounding volume into the light space (via inverse transform), then
      // take the min & max X, Y, Z bounding sphere extents.

      // Get from world to light view space  (world space, origin of {0,0,0}; just rotated)
      auto worldToLightViewMatrix = SSE::Matrix4(lightEntity->worldRotation(), SSE::Vector3(0));
      auto shadowVolumeBoundingBox = mesh_utils::aabbForEntities(
          *_renderableEntities, lightEntity->worldRotation(), [](const Entity& entity) {
            auto* const renderable = entity.getFirstComponentOfType<Renderable_component>();
            assert(renderable != nullptr);

            if (!renderable->castShadow())
              return false;
            ;
            return true;
          });
      shadowData->boundingOrientedBox = shadowVolumeBoundingBox;

      // Now create a shadow camera view matrix. Its position will be the bounds center, offset by
      // {0, 0, extents.z} in light view space.
      const auto lightNearPlaneWorldTranslation =
          SSE::Vector4(shadowVolumeBoundingBox.center, 0.0f) +
          worldToLightViewMatrix * SSE::Vector4(0, 0, shadowVolumeBoundingBox.extents.getZ(), 0);

      Camera_data shadowCameraData;
      {
        auto wtlm = worldToLightViewMatrix;
        const auto pos = SSE::Point3(lightNearPlaneWorldTranslation.getXYZ());
        const auto forward = wtlm * math::forward;
        const auto up = wtlm * math::up;

        shadowCameraData.viewMatrix =
            SSE::Matrix4::lookAt(pos, pos + forward.getXYZ(), up.getXYZ());

        auto extents = shadowVolumeBoundingBox.extents;

        if (extents.getX() == 0.0f)
          extents = SSE::Vector3(0.5f);
        shadowCameraData.projectionMatrix = SSE::Matrix4::orthographic(
            -extents.getX(),
            extents.getX(),
            -extents.getY(),
            extents.getY(),
            -extents.getZ() * 2,
            extents.getZ() * 2.0f);

        // Disable rendering of pixel shader when drawing objects into the shadow camera.
        shadowCameraData.enablePixelShader = false;
      }
      shadowData->worldViewProjMatrix = shadowCameraData.projectionMatrix * shadowCameraData.viewMatrix;

      // Now do the actual rendering to the shadow map
      auto context = d3DDeviceResources.GetD3DDeviceContext();

      auto& shadowMapTexture = D3D_texture_manager::verifyAsD3dShadowMapTexture(*shadowData->shadowMap);

      auto* const depthStencilView = shadowMapTexture.depthStencilView();

      // note that there are NO render target views - we are only rendering to the depth buffer.
      context->OMSetRenderTargets(
          static_cast<UINT>(_renderTargetViews.size()),
          _renderTargetViews.data(),
          depthStencilView);

      context->ClearDepthStencilView(
          depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

      auto dxViewport = CD3D11_VIEWPORT(
          shadowData->viewport.topLeftX,
          shadowData->viewport.topLeftX,
          shadowData->viewport.width,
          shadowData->viewport.height,
          shadowData->viewport.minDepth,
          shadowData->viewport.maxDepth);
      context->RSSetViewports(1, &dxViewport);

      auto& entityRenderManager = _scene.manager<IEntity_render_manager>();

      for (const auto& entity : *_renderableEntities) {
        auto* const renderable = entity->getFirstComponentOfType<Renderable_component>();
        assert(renderable != nullptr);

        if (!renderable->castShadow())
          continue;

        if (shadowVolumeBoundingBox.contains(entity->boundSphere())) {
          entityRenderManager.renderEntity(
              *renderable,
              shadowCameraData,
              Light_provider::no_light_provider,
              Render_pass_blend_mode::Opaque);
        }
      }
    }
  }
}
