#include "pch.h"
#include "Entity_render_manager.h"
#include "Scene_graph_manager.h"
#include "Renderer_data.h"
#include "Shadow_map_texture.h"
#include "Material.h"
#include "Entity.h"
#include "Entity_filter.h"
#include "Scene.h"

#include "Light_component.h"
#include "Clear_gbuffer_material.h"
#include "Render_target_texture.h"
#include "Render_light_data.h"
#include "Renderable_component.h"
#include "Deferred_light_material.h"
#include "Unlit_material.h"
#include "Texture.h"
#include "Camera_component.h"
#include "Entity_sorter.h"
#include "CommonStates.h"
#include "GeometricPrimitive.h"

#include <set>
#include <array>
#include <functional>
#include <optional>

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

// Static initialization order means we can't use Matrix::Identity here.
const Entity_render_manager::Camera_data Entity_render_manager::Camera_data::identity = { 
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },
};

constexpr size_t g_entity_deferred_cleargbuffer = 0;
constexpr size_t g_entity_deferred_geometry = 1;
constexpr size_t g_entity_deferred_lights = 2;

constexpr size_t g_max_render_target_views = 3;

Entity_render_manager::Renderable::Renderable()
	: meshData(nullptr)
	, material(nullptr)
	, rendererData(nullptr)
{
}

using namespace std;

Entity_render_manager::Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository, DX::DeviceResources& deviceResources)
	: IEntity_render_manager(scene)
	, _deviceResources(deviceResources)
	, _renderableEntities(nullptr)
	, _materialRepository(std::move(materialRepository))
	, _primitiveMeshDataFactory(nullptr)
	, _enableDeferredRendering(true)
	, _fatalError(false)
	, _renderPass_shadowMap({})
	, _renderPass_entityDeferred({})
	, _renderPass_entityStandard({})
	, _renderPass_debugElements({})
{
}

void Entity_render_manager::initialize()
{
	using namespace std::placeholders;

	_renderableEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({ Renderable_component::type() });

	_lightEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({
		Directional_light_component::type(),
		Point_light_component::type(),
		Ambient_light_component::type()
	}, Entity_filter_mode::Any);

	_primitiveMeshDataFactory = std::make_unique<Primitive_mesh_data_factory>();
	_alphaSorter = std::make_unique<Entity_alpha_sorter>();
	_cullSorter = std::make_unique<Entity_cull_sorter>();

	createRenderSteps();
}

void Entity_render_manager::tick() 
{
	const auto viewport = _deviceResources.GetScreenViewport();
	const auto aspectRatio = viewport.Width / viewport.Height;
	float invFarPlane;

	const auto cameraEntity = _scene.mainCamera();
	if (cameraEntity)
	{
		// TODO: Replace this with a 'scriptable' component on the camera
		//cameraEntity->LookAt({ 5, 0, 5 }, Vector3::Up);
		auto* component = cameraEntity->getFirstComponentOfType<Camera_component>();
		assert(!!component);

		auto projMatrix = Matrix::CreatePerspectiveFieldOfView(
			component->fov(), 
			aspectRatio,
			component->nearPlane(), 
			component->farPlane());

		if (component->farPlane() == 0.0f)
			invFarPlane = 0.0f;
		else
			invFarPlane = 1.0f / component->farPlane();

		// Construct camera axis vectors, to create a view matrix using lookAt.
		const auto wt = cameraEntity->worldTransform();
		const auto pos = wt.Translation();
		const auto forward = Vector3::TransformNormal(Vector3::Forward, wt);
		const auto up = Vector3::TransformNormal(Vector3::Up, wt);

		auto viewMatrix = Matrix::CreateLookAt(pos, pos + forward, up);
		_cameraData = {
			wt,
			viewMatrix,
			projMatrix
		};
	}
	else
	{
		// Create a default camera
		constexpr auto defaultFarPlane = 30.0f;
		_cameraData = {
			Matrix::Identity,
			Matrix::CreateLookAt({ 0.0f, 0.0f, 10.0f }, Vector3::Zero, Vector3::Up),
			Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(60.0f), aspectRatio, 0.1f, defaultFarPlane)
		};
		invFarPlane = 1.0f / defaultFarPlane;
	}

	_cameraData.projectionMatrix._33 *= invFarPlane;
	_cameraData.projectionMatrix._43 *= invFarPlane;
}

void Entity_render_manager::shutdown()
{
	if (_primitiveMeshDataFactory)
		_primitiveMeshDataFactory.reset();
	
	_renderPass_entityDeferred.data.reset();
	
	_depthTexture.reset();

	auto context = _deviceResources.GetD3DDeviceContext();
	std::array<ID3D11RenderTargetView*, g_max_render_target_views> renderTargetViews = { nullptr, nullptr, nullptr };
	context->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.size()), renderTargetViews.data(), nullptr);
}

void Entity_render_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{	
	auto device = _deviceResources.GetD3DDevice();
	_commonStates = std::make_unique<CommonStates>(device);

	// Shadow map step resources
	_renderPass_shadowMap.data = std::make_shared<decltype(_renderPass_shadowMap.data)::element_type>();
		
	// Deferred lighting step resources
	_renderPass_entityDeferred.data = std::make_shared<decltype(_renderPass_entityDeferred.data)::element_type>();

	std::shared_ptr<Deferred_light_material> deferredLightMaterial;
	deferredLightMaterial.reset(new Deferred_light_material());

	_renderPass_entityDeferred.data->_deferredLightMaterial = deferredLightMaterial;

	_renderPass_entityDeferred.data->_pass0ScreenSpaceQuad = initScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
	_renderPass_entityDeferred.data->_pass2ScreenSpaceQuad = initScreenSpaceQuad(deferredLightMaterial);

	_pbrMaterial_deferred_renderLightData = std::make_unique<decltype(_pbrMaterial_deferred_renderLightData)::element_type>(device);
	_deferredLightMaterial_renderLightData = std::make_unique<decltype(_deferredLightMaterial_renderLightData)::element_type>(device);

	// Standard lighting step resources
	_pbrMaterial_forward_renderLightData = std::make_unique<decltype(_pbrMaterial_forward_renderLightData)::element_type>(device);

	// Debug elements lighting step resources
	_renderPass_debugElements.data = std::make_shared<decltype(_renderPass_debugElements.data)::element_type>();
	_renderPass_debugElements.data->_unlitMaterial = std::make_shared<Unlit_material>();
}

void Entity_render_manager::createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND /*window*/, int width, int height)
{
	const auto d3dDevice = _deviceResources.GetD3DDevice();

	// Step Deferred, Pass 0
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets0;
		renderTargets0.resize(g_max_render_target_views);
		for (auto i = 0; i < renderTargets0.size(); ++i)
		{

			auto target = std::shared_ptr<Render_target_texture>(Render_target_texture::createDefaultRgb(width, height));
			target->load(d3dDevice);
			renderTargets0[i] = std::move(target);
		}

		// Create a depth texture resource
		_depthTexture = std::make_unique<Depth_texture>(_deviceResources);

		// Assign textures to the deferred light material
		auto deferredLightMaterial = _renderPass_entityDeferred.data->_deferredLightMaterial;
		deferredLightMaterial->setColor0Texture(renderTargets0.at(0));
		deferredLightMaterial->setColor1Texture(renderTargets0.at(1));
		deferredLightMaterial->setColor2Texture(renderTargets0.at(2));
		deferredLightMaterial->setDepthTexture(_depthTexture);

		// Give the render targets to the render pass
		std::get<0>(_renderPass_entityDeferred.renderPasses).setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
		std::get<1>(_renderPass_entityDeferred.renderPasses).setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
	}

	// Step Deferred, Pass 2
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
		renderTargets.resize(g_max_render_target_views, nullptr);

		renderTargets[0] = std::make_shared<Render_target_view_texture>(_deviceResources.GetRenderTargetView());
		std::get<2>(_renderPass_entityDeferred.renderPasses).setRenderTargets(std::move(renderTargets));
	}

	// Step Standard, Pass 0
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
		renderTargets.resize(g_max_render_target_views, nullptr);

		renderTargets[0] = std::make_shared<Render_target_view_texture>(_deviceResources.GetRenderTargetView());
		std::get<0>(_renderPass_entityStandard.renderPasses).setRenderTargets(std::move(renderTargets));
	}
}

void Entity_render_manager::destroyDeviceDependentResources()
{
	// Unload shadow maps
	for (const auto& lightEntity : *_lightEntities) {
		// Directional light only, right now
		const auto component = lightEntity->getFirstComponentOfType<Directional_light_component>();
		if (component && component->shadowsEnabled()) {
			component->setShadowData(nullptr);
		}
	}

	_commonStates.reset();
	
	_renderPass_shadowMap.data.reset();
	_renderPass_entityDeferred.data.reset();
	_renderPass_entityStandard.data.reset();
	_renderPass_debugElements.data.reset();

	_clearGBufferMaterial_renderLightData.reset();
	_pbrMaterial_forward_renderLightData.reset();
	_pbrMaterial_deferred_renderLightData.reset();
	_deferredLightMaterial_renderLightData.reset();
	_unlitMaterial_renderLightData.reset();

	_fatalError = false;
}

void Entity_render_manager::destroyWindowSizeDependentResources()
{
	_depthTexture.reset();

	std::get<0>(_renderPass_shadowMap.renderPasses).clearRenderTargets();
	std::get<0>(_renderPass_entityDeferred.renderPasses).clearRenderTargets();
	std::get<1>(_renderPass_entityDeferred.renderPasses).clearRenderTargets();
	std::get<2>(_renderPass_entityDeferred.renderPasses).clearRenderTargets();
	std::get<0>(_renderPass_entityStandard.renderPasses).clearRenderTargets();
	std::get<0>(_renderPass_debugElements.renderPasses).clearRenderTargets();
}

template<uint8_t TMax_lights>
bool addLightToRenderLightData(const Entity& lightEntity, Render_light_data_impl<TMax_lights>& renderLightData)
{
	const auto directionalLight = lightEntity.getFirstComponentOfType<Directional_light_component>();
	if (directionalLight)
	{
		const auto lightDirection = Vector3::Transform(Vector3::Forward, lightEntity.worldRotation());
		return renderLightData.addDirectionalLight(lightDirection, directionalLight->color(), directionalLight->intensity());
	}

	const auto pointLight = lightEntity.getFirstComponentOfType<Point_light_component>();
	if (pointLight)
		return renderLightData.addPointLight(lightEntity.worldPosition(), pointLight->color(), pointLight->intensity());

	const auto ambientLight = lightEntity.getFirstComponentOfType<Ambient_light_component>();
	if (ambientLight)
		return renderLightData.addAmbientLight(ambientLight->color(), ambientLight->intensity());
	return false;
}

void Entity_render_manager::createRenderSteps()
{	
	// Shadow maps
	::std::get<0>(_renderPass_shadowMap.renderPasses).render = [this]() {

		// Render shadow maps for each shadow enabled light
		for (const auto& lightEntity : *_lightEntities) {
			// Directional light only, right now
			const auto component = lightEntity->getFirstComponentOfType<Directional_light_component>();
			if (component && component->shadowsEnabled()) {

				// If this is the first time rendering, initialize.
				if (!component->shadowData()) {
					auto shadowMap = std::make_shared<Shadow_map_texture>(256, 256);
					shadowMap->load(_deviceResources.GetD3DDevice());
					component->setShadowData(shadowMap);
				}

				// Iterate over the shadow *receivers* and build an orthographic frustum from that.

				// Project the receiver bounding volume into the light space (via inverse transform), then take the min & max X, Y, Z bounding sphere extents.

				// Get from world to light view space  (world space, origin of {0,0,0}; just rotated)
				auto worldToLightViewMatrix = Matrix::CreateFromQuaternion(lightEntity->worldRotation());

				BoundingBox bb;

				// Extents, in light view space (as defined above)
				XMVECTOR minExtents = Vector3::Zero;
				XMVECTOR maxExtents = Vector3::Zero;

				for (auto& entity : *_renderableEntities) {
					const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
					assert(renderable != nullptr);

					if (!renderable->castShadow())
						continue;

					const auto& boundSphere = entity->boundSphere();
					Vector3 boundWorldCenter = Vector3::Transform(boundSphere.Center, entity->worldTransform());
					Vector3 boundWorldEdge = Vector3::Transform(Vector3(boundSphere.Center) + Vector3(0, 0, boundSphere.Radius), entity->worldTransform());

					// Bounds, in light view space (as defined above)
					Vector3 boundCenter = Vector3::Transform(boundWorldCenter, XMMatrixInverse(nullptr, worldToLightViewMatrix));
					Vector3 boundEdge = Vector3::Transform(boundWorldEdge, XMMatrixInverse(nullptr, worldToLightViewMatrix));
					const XMVECTOR boundRadius = XMVector3Length(XMVectorSubtract(boundEdge, boundCenter));

					minExtents = XMVectorMin(minExtents, XMVectorSubtract(boundCenter, boundRadius));
					maxExtents = XMVectorMax(maxExtents, XMVectorAdd(boundCenter, boundRadius));
				}

				const auto extents = XMVectorMultiply(XMVectorSubtract(maxExtents, minExtents), XMVectorSet(0.5f, 0.5f, 0.5f, 0.0f));
				bb.Extents = Vector3(extents);
				bb.Center = Vector3(XMVectorAdd(minExtents, extents));

				auto shadowVolumeBoundingBox = BoundingOrientedBox(bb.Center, Vector3(extents), lightEntity->worldRotation());
				component->shadowData()->setCasterVolume(shadowVolumeBoundingBox);

				// Now create a shadow camera view matrix. Its position will be the bounds center, offset by {0, 0, extents.z} in light view space.
				const auto lightWorldTransformMatrix = XMMatrixTranslation(bb.Center.x, bb.Center.y, bb.Center.z + bb.Extents.z);

				Camera_data shadowCameraData;
				shadowCameraData.viewMatrix = XMMatrixMultiply(lightWorldTransformMatrix, worldToLightViewMatrix);
				shadowCameraData.projectionMatrix = Matrix::CreateOrthographic(bb.Extents.x, bb.Extents.y, 0.0f, bb.Extents.z * 2.0f);

				// Now do the actual rendering to the shadow map!
				auto context = _deviceResources.GetD3DDeviceContext();
				const auto depthStencilView = component->shadowData()->depthStencilView();
				std::array<ID3D11RenderTargetView*, g_max_render_target_views> renderTargetViews = {
					nullptr,
					nullptr,
					nullptr
				};
				context->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.size()), renderTargetViews.data(), depthStencilView);
				context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
				Buffer_array_set bas;

				const auto lightDataProvider = [this](const Entity&) {
					_pbrMaterial_deferred_renderLightData->clear();
					return _pbrMaterial_deferred_renderLightData.get();
				};

				for (auto& entity : *_renderableEntities) {
					const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
					assert(renderable != nullptr);

					if (!renderable->castShadow())
						continue;

					if (shadowVolumeBoundingBox.Contains(entity->boundSphere())) {
						render(entity.get(), bas, shadowCameraData, lightDataProvider, std::get<0>(_renderPass_shadowMap.renderPasses));
					}
				}
			}
		}
	};

	// Deferred Lighting
	std::get<0>(_renderPass_entityDeferred.renderPasses).render = [this]() {
		auto renderPass = std::get<0>(_renderPass_entityDeferred.renderPasses);

		// Clear the rendered textures (ignoring depth)
		_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step0_draw");
		const auto& quad = _renderPass_entityDeferred.data->_pass0ScreenSpaceQuad;
		if (quad.rendererData && quad.material) {
			const auto identity = XMMatrixIdentity();
			Buffer_array_set bufferArraySet;

			try
			{
				drawRendererData(Camera_data::identity,
					identity,
					*quad.rendererData,
					renderPass.blendMode(),
					*_clearGBufferMaterial_renderLightData,
					*quad.material,
					false,
					bufferArraySet);
			}
			catch (std::runtime_error& e)
			{
				_fatalError = true;
				LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
			}
		}
		_deviceResources.PIXEndEvent();
	};

	std::get<1>(_renderPass_entityDeferred.renderPasses).render = [this]() {
		auto renderPass = std::get<1>(_renderPass_entityDeferred.renderPasses);

		if (_enableDeferredRendering) {
			_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step1_draw");
			const auto lightDataProvider = [this](const Entity&) { return _pbrMaterial_deferred_renderLightData.get(); };

			_cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
				auto bufferArraySet = Buffer_array_set();
				for (const auto& entry : entities) {
					++_renderStats.opaqueEntityCount;
					render(entry.entity, bufferArraySet, _cameraData, lightDataProvider, renderPass);
				}
			});

			_deviceResources.PIXEndEvent();
		}
	};
	
	std::get<2>(_renderPass_entityDeferred.renderPasses).render = [this]() {
		_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_setup");

		// TODO: Clear the render target view in a more generic way.	 
		_deviceResources.GetD3DDeviceContext()->ClearRenderTargetView(_deviceResources.GetRenderTargetView(), Colors::Black);

		_deviceResources.PIXEndEvent();

		if (!_fatalError)
		{
			if (_enableDeferredRendering) {
				_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_draw");
				renderLights();
				_deviceResources.PIXEndEvent();
			}
		}
	};

	// Standard Lighting pass
	std::get<0>(_renderPass_entityStandard.renderPasses).render = [this]() {
		auto renderPass = std::get<0>(_renderPass_entityStandard.renderPasses);

		const auto standardLightDataProvider = [this](const Entity&) {
			_pbrMaterial_forward_renderLightData->clear();
			for (auto iter = _lightEntities->begin(); iter != _lightEntities->end(); ++iter) {
				if (_pbrMaterial_forward_renderLightData->full())
					break;

				addLightToRenderLightData(*iter->get(), *_pbrMaterial_forward_renderLightData);
			}
			_pbrMaterial_forward_renderLightData->updateBuffer(_deviceResources.GetD3DDeviceContext());

			return _pbrMaterial_forward_renderLightData.get();
		};

		_alphaSorter->waitThen([this, &standardLightDataProvider, &renderPass](const std::vector<Entity_alpha_sorter_entry>& entries) {
			if (!_fatalError)
			{
				_deviceResources.PIXBeginEvent(L"renderPass_EntityStandard_draw");

				_cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
					auto bufferArraySet = Buffer_array_set();
					for (const auto& entry : entities) {
						++_renderStats.alphaEntityCount;
						render(entry.entity, bufferArraySet, _cameraData, standardLightDataProvider, renderPass);
					}
				});

				_deviceResources.PIXEndEvent();
			}
		});
	};

	std::get<0>(_renderPass_debugElements.renderPasses).render = [this]() {
		auto unlitMaterial = _renderPass_debugElements.data->_unlitMaterial;
		Buffer_array_set bufferArraySet;
		for (const auto& debugShape : _renderPass_debugElements.data->_debugShapes) {
			const auto& transform = std::get<0>(debugShape);
			const auto& color = std::get<1>(debugShape);
			const auto rendererData = std::get<2>(debugShape);

			unlitMaterial->setBaseColor(color);

			drawRendererData(
				_cameraData,
				transform,
				*rendererData,
				Render_pass_blend_mode::Opaque,
				*_unlitMaterial_renderLightData,
				*unlitMaterial,
				true,
				bufferArraySet);
		}
	};
}

void Entity_render_manager::clearDepthStencil(float depth, uint8_t stencil) const
{
	const auto depthStencil = _deviceResources.GetDepthStencilView();
	_deviceResources.GetD3DDeviceContext()->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void Entity_render_manager::render()
{
	const auto cameraEntity = _scene.mainCamera();
	if (!cameraEntity)
		return;

	if (_fatalError)
		return;

	auto bufferArraySet = Buffer_array_set();
	
	const auto frustum = createFrustum(*cameraEntity.get(), *cameraEntity->getFirstComponentOfType<Camera_component>());
	_cullSorter->beginSortAsync(_renderableEntities->begin(), _renderableEntities->end(), frustum);

	// Block on the cull sorter, since we can't render until it is done; and it is a good place to kick off the alpha sort.
	_cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
		// Find the list of entities that have alpha, and sort them.
		_alphaSorter->beginSortAsync(entities.begin(), entities.end(), cameraEntity->worldPosition());

		// Render steps
		_renderStats = {};
		clearDepthStencil();
		renderStep(_renderPass_shadowMap);
		renderStep(_renderPass_entityDeferred);
		renderStep(_renderPass_entityStandard);
		renderStep(_renderPass_debugElements);
	});
}

template<int TIdx, class TData, class... TRender_passes>
void Entity_render_manager::renderStep(Render_step<TData, TRender_passes...>& step)
{
	if (!step.enabled)
		return;

	auto pass = std::get<TIdx>(step.renderPasses);

	auto[renderTargetViews, numRenderTargets] = pass.renderTargetViewArray();
	if (numRenderTargets) {
		auto context = _deviceResources.GetD3DDeviceContext();

		// TODO: Fix me!!!

		if constexpr (Render_pass_depth_mode::Disabled == pass.depthMode())
			context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, nullptr);
		else
			context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, _deviceResources.GetDepthStencilView());
	}

	renderPass(pass);

	if constexpr (TIdx + 1 < sizeof...(TRender_passes)) {
		renderStep<TIdx + 1, TData, TRender_passes...>(step);
	}
}

template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
void Entity_render_manager::renderPass(Render_pass_info<TBlend_mode, TDepth_mode>& renderPassInfo)
{
	auto context = _deviceResources.GetD3DDeviceContext();

	// Set the blend mode
	constexpr auto opaqueSampleMask = 0xffffffff;
	constexpr std::array<float, 4> opaqueBlendFactor{ 0.0f, 0.0f, 0.0f, 0.0f };

	if constexpr (TBlend_mode == Render_pass_blend_mode::Opaque)
		context->OMSetBlendState(_commonStates->Opaque(), opaqueBlendFactor.data(), opaqueSampleMask);
	else if constexpr (TBlend_mode == Render_pass_blend_mode::Blended_alpha)
		context->OMSetBlendState(_commonStates->AlphaBlend(), opaqueBlendFactor.data(), opaqueSampleMask);
	else if constexpr (TBlend_mode == Render_pass_blend_mode::Additive)
		context->OMSetBlendState(_commonStates->Additive(), opaqueBlendFactor.data(), opaqueSampleMask);
	else
		static_assert(false);

	// Depth buffer mode
	if constexpr (Render_pass_depth_mode::ReadWrite == renderPassInfo.depthMode())
		context->OMSetDepthStencilState(_commonStates->DepthDefault(), 0);
	else if constexpr (Render_pass_depth_mode::ReadOnly == renderPassInfo.depthMode())
		context->OMSetDepthStencilState(_commonStates->DepthRead(), 0);
	else
		context->OMSetDepthStencilState(_commonStates->DepthNone(), 0);

	// Make sure wire-frame is disabled
	context->RSSetState(_commonStates->CullClockwise());

	// Set the viewport.
	auto viewport = _deviceResources.GetScreenViewport();
	_deviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);

	// Call the render method.
	renderPassInfo.render();
}

BoundingFrustumRH Entity_render_manager::createFrustum(const Entity& entity, const Camera_component& cameraComponent)
{
	const auto viewport = _deviceResources.GetScreenViewport();
	const auto aspectRatio = viewport.Width / viewport.Height;

	const auto projMatrix = Matrix::CreatePerspectiveFieldOfView(
		cameraComponent.fov(),
		aspectRatio,
		cameraComponent.nearPlane(),
		cameraComponent.farPlane());
	auto frustum = BoundingFrustumRH(projMatrix);
	frustum.Origin = entity.worldPosition();
	frustum.Orientation = entity.worldRotation();

	return frustum;
}

template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
void Entity_render_manager::render(Entity* entity,
	Buffer_array_set& bufferArraySet,
	const Camera_data& cameraData,
	const Light_data_provider& lightDataProvider,
	const Render_pass_info<TBlend_mode, TDepth_mode>& renderPassInfo)
{
	auto renderable = entity->getFirstComponentOfType<Renderable_component>();
	if (!renderable->visible())
		return;

	try {
		const auto material = renderable->material().get();
		assert(material != nullptr);

		// Check that this render pass supports this materials alpha mode
		if constexpr (TBlend_mode == Render_pass_blend_mode::Blended_alpha) {
			if (material->getAlphaMode() != Material_alpha_mode::Blend)
				return;
		}
		else {
			if (material->getAlphaMode() == Material_alpha_mode::Blend)
				return;
		}
	
		const Renderer_data* rendererData = renderable->rendererData().get();
		if (rendererData == nullptr) {

			const auto meshDataComponent = entity->getFirstComponentOfType<Mesh_data_component>();
			if (meshDataComponent == nullptr || meshDataComponent->meshData() == nullptr)
			{
				// There is no mesh data (it may still be loading!), we can't render.
				return;
			}
			const auto& meshData = meshDataComponent->meshData();

			LOG(INFO) << "Creating renderer data for entity " << entity->getName() << " (ID " << entity->getId() << ")";

			std::vector<Vertex_attribute> vertexAttributes;
			material->vertexAttributes(vertexAttributes);

			auto rendererDataPtr = createRendererData(*meshData, vertexAttributes);
			rendererData = rendererDataPtr.get();
			renderable->setRendererData(std::move(rendererDataPtr));
		}

		const auto renderLightData = lightDataProvider(*entity);
		if (!renderLightData)
			throw std::runtime_error("Could not get Render_light_data");

		drawRendererData(
			cameraData,
			entity->worldTransform(),
			*rendererData,
			renderPassInfo.blendMode(),
			*renderLightData,
			*material,
			renderable->wireframe(),
			bufferArraySet);
	} 
	catch (std::runtime_error& e)
	{
		renderable->setVisible(false);
		LOG(WARNING) << "Failed to render mesh on entity " << entity->getName() << " (ID " << entity->getId() << "): " << e.what();
	}
}

void Entity_render_manager::renderLights()
{
	auto context = _deviceResources.GetD3DDeviceContext();
	auto bufferArraySet = Buffer_array_set();

	try
	{
		const auto& quad = _renderPass_entityDeferred.data->_pass2ScreenSpaceQuad;
		const auto rendererData = quad.rendererData.get();
		const auto deferredLightMaterial = _renderPass_entityDeferred.data->_deferredLightMaterial;

		if (!rendererData || !rendererData->vertexCount)
			return;

		const auto renderLight = [&]() {
			++_renderStats.opaqueLightCount;
			_deferredLightMaterial_renderLightData->updateBuffer(context);
			const auto renderSuccess = quad.material->render(
				*rendererData,
				std::get<g_entity_deferred_lights>(_renderPass_entityDeferred.renderPasses).blendMode(),
				*_deferredLightMaterial_renderLightData,
				_cameraData.worldMatrix,
				_cameraData.viewMatrix,
				_cameraData.projectionMatrix,
				_deviceResources);
			if (!renderSuccess)
				throw std::runtime_error("Failed to render deferred light");

			_deferredLightMaterial_renderLightData->clear();
		};

		loadRendererDataToDeviceContext(*rendererData, bufferArraySet);

		// Render emitted light sources once only.
		deferredLightMaterial->setupEmitted(true);
		auto renderedEmitted = false;

		// Render directional, point, ambient
		for (auto eIter = _lightEntities->begin(); eIter != _lightEntities->end(); ++eIter)
		{
			addLightToRenderLightData(*eIter->get(), *_deferredLightMaterial_renderLightData);

			if (_deferredLightMaterial_renderLightData->full()) {
				renderLight();
				renderedEmitted = true;
				deferredLightMaterial->setupEmitted(false);
			}
		}

		if (!_deferredLightMaterial_renderLightData->empty() || !renderedEmitted)
			renderLight();

		// Unbind material
		quad.material->unbind(_deviceResources);
	}
	catch (const std::runtime_error& e)
	{
		_fatalError = true;
		LOG(WARNING) << "Failed to render lights.\n" << e.what();
	}
}

void Entity_render_manager::loadRendererDataToDeviceContext(const Renderer_data& rendererData, Buffer_array_set& bufferArraySet) const
{
	const auto deviceContext = _deviceResources.GetD3DDeviceContext();

	// Send the buffers to the input assembler
	if (!rendererData.vertexBuffers.empty()) {
		auto& vertexBuffers = rendererData.vertexBuffers;
		const auto numBuffers = vertexBuffers.size();
		if (numBuffers > 1) {
			bufferArraySet.bufferArray.clear();
			bufferArraySet.strideArray.clear();
			bufferArraySet.offsetArray.clear();

			for (std::vector<Mesh_buffer_accessor>::size_type i = 0; i < numBuffers; ++i) {
				const D3D_buffer_accessor* vertexBufferDesc = rendererData.vertexBuffers[i].get();
				bufferArraySet.bufferArray.push_back(vertexBufferDesc->buffer->d3dBuffer);
				bufferArraySet.strideArray.push_back(vertexBufferDesc->stride);
				bufferArraySet.offsetArray.push_back(vertexBufferDesc->offset);
			}

			deviceContext->IASetVertexBuffers(0, static_cast<UINT>(numBuffers), bufferArraySet.bufferArray.data(), bufferArraySet.strideArray.data(), bufferArraySet.offsetArray.data());
		}
		else {
			const D3D_buffer_accessor* vertexBufferDesc = rendererData.vertexBuffers[0].get();
			deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDesc->buffer->d3dBuffer, &vertexBufferDesc->stride, &vertexBufferDesc->offset);
		}

		// Set the type of primitive that should be rendered from this vertex buffer.
		deviceContext->IASetPrimitiveTopology(rendererData.topology);

		if (rendererData.indexBufferAccessor != nullptr) {
			// Set the index buffer to active in the input assembler so it can be rendered.
			const auto indexAccessor = rendererData.indexBufferAccessor.get();
			deviceContext->IASetIndexBuffer(indexAccessor->buffer->d3dBuffer, rendererData.indexFormat, indexAccessor->offset);
		}
	}
}

void Entity_render_manager::drawRendererData(
	const Camera_data& cameraData,
	const Matrix& worldTransform,
	const Renderer_data& rendererData,
	const Render_pass_blend_mode blendMode,
	const Render_light_data& renderLightData,
	Material& material,
	bool wireFrame,
	Buffer_array_set& bufferArraySet) const
{
	if (rendererData.vertexBuffers.empty())
		return;

	const auto deviceContext = _deviceResources.GetD3DDeviceContext();
	loadRendererDataToDeviceContext(rendererData, bufferArraySet);
	
	// Set the rasteriser state
	if (wireFrame)
		deviceContext->RSSetState(_commonStates->Wireframe());
	else
		deviceContext->RSSetState(_commonStates->CullClockwise());

	// Render the triangles
	material.render(rendererData, blendMode, renderLightData, 
		worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, 
		_deviceResources);
}


std::unique_ptr<Renderer_data> Entity_render_manager::createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const
{
	auto rendererData = std::make_unique<Renderer_data>();

	switch (meshData.m_meshIndexType) {
	case Mesh_index_type::Triangles:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case Mesh_index_type::Lines:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		throw std::exception("Unsupported mesh topology");
	}

	// Create D3D Index Buffer
	if (meshData.indexBufferAccessor) {
		rendererData->indexCount = meshData.indexBufferAccessor->count;

		rendererData->indexBufferAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData(*meshData.indexBufferAccessor->buffer, D3D11_BIND_INDEX_BUFFER),
			meshData.indexBufferAccessor->stride,
			meshData.indexBufferAccessor->offset);
		rendererData->indexFormat = meshData.indexBufferAccessor->format;

		std::string name("Index Buffer (count: " + std::to_string(rendererData->indexCount) + ")");
		rendererData->indexBufferAccessor->buffer->d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else 
	{
		// TODO: Simply log a warning?
		rendererData->indexCount = 0;
		throw std::runtime_error("CreateRendererData: Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->vertexCount = meshData.getVertexCount();
	for (auto vertexAttr : vertexAttributes)
	{
		const auto vabaPos = meshData.vertexBufferAccessors.find(vertexAttr);
		if (vabaPos == meshData.vertexBufferAccessors.end())
			throw std::runtime_error("CreateRendererData: Missing vertex attribute: "s.append(Vertex_attribute_meta::semanticName(vertexAttr)));

		const auto& meshAccessor = vabaPos->second;
		auto d3dAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData(*meshAccessor->buffer, D3D11_BIND_VERTEX_BUFFER),
			meshAccessor->stride, 
			meshAccessor->offset);

		rendererData->vertexBuffers.push_back(std::move(d3dAccessor));
	}

	return rendererData;
}

Entity_render_manager::Renderable Entity_render_manager::initScreenSpaceQuad(std::shared_ptr<Material> material) const
{
	Renderable renderable;
	if (renderable.meshData == nullptr)
		renderable.meshData = _primitiveMeshDataFactory->createQuad({ 2.0f, 2.0f }, { -1.f, -1.f, 0.f });

	if (renderable.material == nullptr)
		renderable.material = material;

	if (renderable.rendererData == nullptr) 
	{
		std::vector<Vertex_attribute> vertexAttributes;
		renderable.material->vertexAttributes(vertexAttributes);
		renderable.rendererData = createRendererData(*renderable.meshData, vertexAttributes);
	}

	return renderable;
}

std::shared_ptr<D3D_buffer> Entity_render_manager::createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const
{
	// Create the vertex buffer.
	auto d3dBuffer = std::make_shared<D3D_buffer>();

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = static_cast<UINT>(buffer.dataSize);
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the sub-resource data.
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = buffer.data;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	// Get a reference
	const auto hr = _deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &initData, &d3dBuffer->d3dBuffer);
	assert(!FAILED(hr));

	return d3dBuffer;
}

std::unique_ptr<Material> Entity_render_manager::loadMaterial(const std::string& materialName) const
{
	return _materialRepository->instantiate(materialName);
}

void Entity_render_manager::addDebugSphere(const Matrix& worldTransform, float radius, const Color& color)
{
	const auto meshData = _primitiveMeshDataFactory->createSphere(radius, 6);

	std::vector<Vertex_attribute> vertexAttributes;
	_renderPass_debugElements.data->_unlitMaterial->vertexAttributes(vertexAttributes);
	auto rendererData = createRendererData(*meshData, vertexAttributes);
	_renderPass_debugElements.data->_debugShapes.push_back({ worldTransform, color, move(rendererData) });
}

void Entity_render_manager::addDebugBoundingBox(const BoundingOrientedBox& boundingOrientedBox, const Color& color)
{
	const auto meshData = _primitiveMeshDataFactory->createBox(boundingOrientedBox.Extents * 2.0f);

	std::vector<Vertex_attribute> vertexAttributes;
	_renderPass_debugElements.data->_unlitMaterial->vertexAttributes(vertexAttributes);
	auto rendererData = createRendererData(*meshData, vertexAttributes);

	auto worldTransform = XMMatrixAffineTransformation(Vector3::One, Vector3::Zero, XMLoadFloat4(&boundingOrientedBox.Orientation), XMLoadFloat3(&boundingOrientedBox.Center));

	_renderPass_debugElements.data->_debugShapes.push_back({ worldTransform, color, move(rendererData) });
}

void Entity_render_manager::addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const Color& color)
{
	// Create frustum mesh with vertices in WORLD SPACE, based on the position and orientation attributes of the frustum.
	const auto meshData = _primitiveMeshDataFactory->createFrustumLines(boundingFrustum);

	std::vector<Vertex_attribute> vertexAttributes;
	_renderPass_debugElements.data->_unlitMaterial->vertexAttributes(vertexAttributes);
	auto rendererData = createRendererData(*meshData, vertexAttributes);
	
	_renderPass_debugElements.data->_debugShapes.push_back({ Matrix::Identity, color, move(rendererData) });
}

void Entity_render_manager::clearDebugShapes()
{
	if (_renderPass_debugElements.data)
		_renderPass_debugElements.data->_debugShapes.clear();
}