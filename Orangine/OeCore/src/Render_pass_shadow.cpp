#include "pch.h"
#include "OeCore/IShadowmap_manager.h"
#include "OeCore/Light_component.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Shadow_map_texture.h"
#include "OeCore/Mesh_utils.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/Entity.h"
#include "OeCore/Scene.h"
#include "OeCore/Render_pass_shadow.h"
#include "OeCore/Math_constants.h"
#include "D3D11/DirectX_utils.h"

using namespace DirectX;
using namespace oe;

Render_pass_shadow::Render_pass_shadow(Scene& scene, size_t maxRenderTargetViews)
	: _scene(scene)
{
	_renderTargetViews.resize(maxRenderTargetViews, nullptr);

	_renderableEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({ Renderable_component::type() });
	_lightEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({
		Directional_light_component::type(),
		Point_light_component::type(),
		Ambient_light_component::type()
		}, Entity_filter_mode::Any);
}

void Render_pass_shadow::render(const Camera_data&)
{
	auto& d3DDeviceResources = _scene.manager<ID3D_resources_manager>().deviceResources();
	d3DDeviceResources.PIXBeginEvent(L"_renderPass_shadowMap_draw");

	// Render shadow maps for each shadow enabled light
	for (const auto& lightEntity : *_lightEntities) {
		// Directional light only, right now
		const auto component = lightEntity->getFirstComponentOfType<Directional_light_component>();
		if (component && component->shadowsEnabled()) {

			Shadow_map_texture* shadowData = component->shadowData().get();

			// If this is the first time rendering, initialize a new shadowmap.
			if (!shadowData) {
				auto shadowMap = _scene.manager<IShadowmap_manager>().borrowTexture();
				shadowMap->load(d3DDeviceResources.GetD3DDevice());

				shadowData = shadowMap.get();
				component->shadowData() = std::move(shadowMap);
			}

			// Iterate over the shadow *receivers* and build an orthographic frustum from that.

			// Project the receiver bounding volume into the light space (via inverse transform), then take the min & max X, Y, Z bounding sphere extents.

			// Get from world to light view space  (world space, origin of {0,0,0}; just rotated)
			auto worldToLightViewMatrix = DirectX::SimpleMath::Matrix::CreateFromQuaternion(lightEntity->worldRotation());
			auto shadowVolumeBoundingBox = mesh_utils::aabbForEntities(
				*_renderableEntities,
				toQuat(lightEntity->worldRotation()),
				[](const Entity& entity) {
				const auto renderable = entity.getFirstComponentOfType<Renderable_component>();
				assert(renderable != nullptr);

				if (!renderable->castShadow())
					return false;;
				return true;
			});
			shadowData->setCasterVolume(shadowVolumeBoundingBox);

			// Now create a shadow camera view matrix. Its position will be the bounds center, offset by {0, 0, extents.z} in light view space.
			const auto lightNearPlaneWorldTranslation = XMVectorAdd(
				XMVector3Transform(XMVectorSet(0.0f, 0.0f, shadowVolumeBoundingBox.Extents.z, 0.0f), worldToLightViewMatrix),
				XMVectorSet(shadowVolumeBoundingBox.Center.x,
					shadowVolumeBoundingBox.Center.y,
					shadowVolumeBoundingBox.Center.z,
					0.0f)
			);

			Camera_data shadowCameraData;
			{
				auto wtlm = toVectorMathMat4(worldToLightViewMatrix);
				const auto pos = toPoint3(SimpleMath::Vector3(lightNearPlaneWorldTranslation));
				const auto forward = wtlm * Math::Direction::Forward;
				const auto up = wtlm * Math::Direction::Up;

				shadowCameraData.viewMatrix = SSE::Matrix4::lookAt(pos, pos + forward.getXYZ(), up.getXYZ());

				SimpleMath::Vector3 extents2;
				XMStoreFloat3(&extents2, XMVectorScale(XMLoadFloat3(&shadowVolumeBoundingBox.Extents), 2.0f));

				if (extents2.x == 0.0f)
					extents2 = SimpleMath::Vector3::One;
				auto refProjectionMatrix = toVectorMathMat4(SimpleMath::Matrix::CreateOrthographic(extents2.x, extents2.y, 0.0f, extents2.z));


				SSE::Vector3 extents = LoadVector3(shadowVolumeBoundingBox.Extents);

				if (extents.getX() == 0.0f)
					extents = SSE::Vector3(0.5f);
				//SimpleMath::Matrix::CreateOrthographic()
				shadowCameraData.projectionMatrix = SSE::Matrix4::orthographic(-extents.getX(), extents.getX(), -extents.getY(), extents.getY(), -extents.getZ() * 2, extents.getZ() * 2.0f);

				// Disable rendering of pixel shader when drawing objects into the shadow camera.
				shadowCameraData.enablePixelShader = false;
			}
			shadowData->setWorldViewProjMatrix(
				shadowCameraData.projectionMatrix * shadowCameraData.viewMatrix
			);

			// Now do the actual rendering to the shadow map
			auto context = d3DDeviceResources.GetD3DDeviceContext();
			const auto depthStencilView = shadowData->depthStencilView();
			context->OMSetRenderTargets(static_cast<UINT>(_renderTargetViews.size()), _renderTargetViews.data(), depthStencilView);
			
			context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			context->RSSetViewports(1, &shadowData->viewport());

			auto& entityRenderManager = _scene.manager<IEntity_render_manager>();
			for (auto& entity : *_renderableEntities) {
				const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
				assert(renderable != nullptr);

				if (!renderable->castShadow())
					continue;

				if (shadowVolumeBoundingBox.Contains(StoreBoundingSphere(entity->boundSphere()))) {
					entityRenderManager.renderEntity(*renderable, shadowCameraData, Light_provider::no_light_provider, Render_pass_blend_mode::Opaque);
				}
			}
		}
	}

	d3DDeviceResources.PIXEndEvent();
}
