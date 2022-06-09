#include "D3D12_render_pass_shadow.h"

#include <OeCore/Light_component.h>
#include <OeCore/Light_provider.h>
#include <OeCore/Mesh_utils.h>
#include <OeCore/Renderable_component.h>

using namespace oe::pipeline_d3d12;

D3D12_render_pass_shadow::D3D12_render_pass_shadow(
        D3D12_device_resources& deviceResources, IScene_graph_manager& sceneGraphManager,
        IShadowmap_manager& shadowMapManager, D3D12_texture_manager& textureManager,
        IEntity_render_manager& entityRenderManager)
    : _deviceResources(deviceResources)
    , _sceneGraphManager(sceneGraphManager)
    , _shadowMapManager(shadowMapManager)
    , _textureManager(textureManager)
    , _entityRenderManager(entityRenderManager)
{
  _renderableEntities = sceneGraphManager.getEntityFilter({Renderable_component::type()});
  _lightEntities = sceneGraphManager.getEntityFilter(
          {Directional_light_component::type(), Point_light_component::type(), Ambient_light_component::type()},
          Entity_filter_mode::Any);
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE& D3D12_render_pass_shadow::getResourceForSlice(Texture& texture)
{
  // create a dsv for each shadowmap pool slice
  ID3D12Resource* textureResource = _textureManager.getResource(texture);
  UINT arraySize = textureResource->GetDesc().DepthOrArraySize;
  if (_shadowmapSliceResources.size() < arraySize) {
    OE_CHECK(arraySize <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);
    _shadowmapSliceResources.resize(arraySize);
  }
  auto& sliceDescRange = _shadowmapSliceResources.at(texture.getArraySlice());

  if (!sliceDescRange.isValid()) {
    // None exists, create a DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc{};
    depthStencilDesc.Format = _deviceResources.getDepthStencilFormat();
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    if (texture.isArraySlice()) {
      depthStencilDesc.Texture2DArray.ArraySize = 1;
      depthStencilDesc.Texture2DArray.MipSlice = 0;
      depthStencilDesc.Texture2DArray.FirstArraySlice = texture.getArraySlice();
    }
    sliceDescRange = _deviceResources.getDsvHeap().allocateRange(1);
    _deviceResources.GetD3DDevice()->CreateDepthStencilView(
            textureResource, &depthStencilDesc, sliceDescRange.cpuHandle);
  }
  return sliceDescRange.cpuHandle;
}

void D3D12_render_pass_shadow::render(const Camera_data&)
{
  auto arrayTexture = _shadowMapManager.getShadowMapTextureArray();
  if (!arrayTexture) {
    return;
  }
  if (!arrayTexture->isValid()) {
    _textureManager.load(*arrayTexture);
    return;
  }

  ID3D12Resource* arrayTextureResource = _textureManager.getResource(*arrayTexture);
  OE_CHECK(arrayTextureResource);
  auto preRenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
          arrayTextureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
  auto postRenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
          arrayTextureResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  ID3D12GraphicsCommandList* commandList = _deviceResources.GetPipelineCommandList().Get();
  commandList->ResourceBarrier(1, &preRenderBarrier);

  renderShadowMapArray(commandList);

  commandList->ResourceBarrier(1, &postRenderBarrier);
}

void D3D12_render_pass_shadow::renderShadowMapArray(ID3D12GraphicsCommandList* commandList)
{
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
        shadowData->shadowMap = _shadowMapManager.borrowTextureSlice();
      }
      OE_CHECK(shadowData->shadowMap);

      // Iterate over the shadow *receivers* and build an orthographic frustum from that.

      // Project the receiver bounding volume into the light space (via inverse transform), then
      // take the min & max X, Y, Z bounding sphere extents.

      // Get from world to light view space  (world space, origin of {0,0,0}; just rotated)
      auto worldToLightViewMatrix = SSE::Matrix4(lightEntity->worldRotation(), SSE::Vector3(0));
      auto shadowVolumeBoundingBox =
              mesh_utils::aabbForEntities(*_renderableEntities, lightEntity->worldRotation(), [](const Entity& entity) {
                auto* const renderable = entity.getFirstComponentOfType<Renderable_component>();
                assert(renderable != nullptr);

                if (!renderable->castShadow()) {
                  return false;
                }
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

        shadowCameraData.viewMatrix = SSE::Matrix4::lookAt(pos, pos + forward.getXYZ(), up.getXYZ());

        auto extents = shadowVolumeBoundingBox.extents;

        if (extents.getX() == 0.0f) extents = SSE::Vector3(0.5f);
        shadowCameraData.projectionMatrix = SSE::Matrix4::orthographic(
                -extents.getX(), extents.getX(), -extents.getY(), extents.getY(), -extents.getZ() * 2,
                extents.getZ() * 2.0f);

        // Disable rendering of pixel shader when drawing objects into the shadow camera.
        shadowCameraData.enablePixelShader = false;
      }
      shadowData->worldViewProjMatrix = shadowCameraData.projectionMatrix * shadowCameraData.viewMatrix;

      // Now do the actual rendering to the shadow map
      ID3D12Device4* device = _deviceResources.GetD3DDevice();

      Texture& shadowMapTexture = *shadowData->shadowMap;
      if (shadowMapTexture.isValid()) {
        // Not ready to render yet.
        _textureManager.load(shadowMapTexture);
        continue;
      }

      const auto& depthStencilView = getResourceForSlice(shadowMapTexture);

      // note that there are no render target views since we are only rendering to the depth buffer.
      commandList->OMSetRenderTargets(0, nullptr, false, &depthStencilView);
      commandList->ClearDepthStencilView(
              depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

      const auto viewportWidth = static_cast<float>(shadowMapTexture.getDimension().x);
      const auto viewportHeight = static_cast<float>(shadowMapTexture.getDimension().y);
      const auto dxViewport = CD3DX12_VIEWPORT(0.f, 0.f, viewportWidth, viewportHeight);
      commandList->RSSetViewports(1, &dxViewport);

      for (const auto& entity : *_renderableEntities) {
        auto* const renderable = entity->getFirstComponentOfType<Renderable_component>();
        assert(renderable != nullptr);

        if (!renderable->castShadow()) {
          continue;
        }

        if (shadowVolumeBoundingBox.contains(entity->boundSphere())) {
          _entityRenderManager.renderEntity(
                  *renderable, shadowCameraData, Light_provider::no_light_provider, this->getDepthStencilConfig(),
                  Render_pass_target_layout::None);
        }
      }
    }
  }
}
