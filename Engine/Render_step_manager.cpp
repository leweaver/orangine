#include "pch.h"
#include "Render_step_manager.h"
#include "Deferred_light_material.h"
#include "Clear_gbuffer_material.h"
#include "Render_pass_shadow.h"
#include "Entity_sorter.h"
#include "Camera_component.h"
#include "Entity_render_manager.h"
#include "Render_pass_generic.h"
#include "Renderable.h"
#include "Scene.h"
#include "Entity.h"
#include "Light_component.h"
#include "Shadow_map_texture_pool.h"
#include "CommonStates.h"
#include "Dev_tools_manager.h"
#include "Render_pass_skybox.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;

constexpr size_t g_max_render_target_views = 3;

Render_step_manager::Render_step_manager(Scene & scene)
	: IRender_step_manager(scene)
	, _renderStep_shadowMap({})
	, _renderStep_entityDeferred({})
	, _renderStep_entityStandard({})
	, _renderStep_debugElements({})
	, _renderStep_skybox({})
	, _fatalError(false)
	, _enableDeferredRendering(true)
{
	_simpleLightProvider = [this](const BoundingSphere& target, std::vector<Entity*>& lights, uint32_t maxLights) {
		for (auto iter = _lightEntities->begin(); iter != _lightEntities->end(); ++iter) {
			if (lights.size() >= maxLights) { break; }
			lights.push_back(iter->get());
		}
	};
}

inline DX::DeviceResources& Render_step_manager::deviceResources() const
{
	return _scene.manager<ID3D_resources_manager>().deviceResources();
}

void Render_step_manager::initialize()
{
	using namespace std::placeholders;

	_renderableEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({ Renderable_component::type() });
	_lightEntities = _scene.manager<IScene_graph_manager>().getEntityFilter({
		Directional_light_component::type(),
		Point_light_component::type(),
		Ambient_light_component::type()
		}, Entity_filter_mode::Any);

	_alphaSorter = std::make_unique<Entity_alpha_sorter>();
	_cullSorter = std::make_unique<Entity_cull_sorter>();

	createRenderSteps();
}

void Render_step_manager::shutdown()
{
	_renderStep_entityDeferred.data.reset();

	auto context = deviceResources().GetD3DDeviceContext();
	std::array<ID3D11RenderTargetView*, g_max_render_target_views> renderTargetViews = { nullptr, nullptr, nullptr };
	context->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.size()), renderTargetViews.data(), nullptr);

	_renderStep_shadowMap.renderPasses[0].reset();
	_renderStep_entityDeferred.renderPasses[0].reset();
	_renderStep_entityDeferred.renderPasses[1].reset();
	_renderStep_entityDeferred.renderPasses[2].reset();
	_renderStep_entityStandard.renderPasses[0].reset();
	_renderStep_debugElements.renderPasses[0].reset();
	_renderStep_skybox.renderPasses[0].reset();

	_renderableEntities.reset();
	_lightEntities.reset();
}


void Render_step_manager::createRenderSteps()
{
	// Shadow maps
	_renderStep_shadowMap.renderPasses[0] = std::make_unique<Render_pass_shadow>(_scene, g_max_render_target_views);
	_renderStep_shadowMap.renderPasses[0]->setStencilRef(1);

	// Deferred Lighting
	_renderStep_entityDeferred.renderPasses[0] = std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
		auto renderPassConfig = std::get<0>(_renderStep_entityDeferred.renderPassConfigs);
		auto& entityRenderManager = _scene.manager<IEntity_render_manager>();

		// Clear the rendered textures (ignoring depth)
		auto& d3DDeviceResources = deviceResources();
		d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step0");
		auto& quad = _renderStep_entityDeferred.data->pass0ScreenSpaceQuad;
		if (quad.rendererData && quad.material) {
			try
			{
				entityRenderManager.renderRenderable(
					quad,
					Matrix::Identity,
					0.0f,
					Render_pass::Camera_data::identity,
					Light_provider::no_light_provider,
					renderPassConfig.blendMode(),
					false);
			}
			catch (std::runtime_error& e)
			{
				_fatalError = true;
				LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
			}
		}
		d3DDeviceResources.PIXEndEvent();
	});

	_renderStep_entityDeferred.renderPasses[1] = std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
		auto renderPassConfig = std::get<1>(_renderStep_entityDeferred.renderPassConfigs);

		if (_enableDeferredRendering) {
			auto& d3DDeviceResources = deviceResources();
			d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step1");

			_cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
				for (const auto& entry : entities) {
					// TODO: Stats
					// ++_renderStats.opaqueEntityCount;
					renderEntity(entry.entity, cameraData, Light_provider::no_light_provider, renderPassConfig);
				}
			});

			d3DDeviceResources.PIXEndEvent();
		}
	});

	_renderStep_entityDeferred.renderPasses[2] = std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
		const auto blendMode = std::get<2>(_renderStep_entityDeferred.renderPassConfigs).blendMode();
		
		auto& d3DDeviceResources = deviceResources();
		d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_setup");

		// TODO: Clear the render target view in a more generic way.	 
		d3DDeviceResources.GetD3DDeviceContext()->ClearRenderTargetView(d3DDeviceResources.GetRenderTargetView(), Colors::Black);

		d3DDeviceResources.PIXEndEvent();

		if (!_fatalError)
		{
			if (_enableDeferredRendering) {
				d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_draw");
				renderLights(cameraData, blendMode);
				d3DDeviceResources.PIXEndEvent();
			}
		}
	});

	// Standard Lighting pass
	_renderStep_entityStandard.renderPasses[0] = std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
		const auto& renderPassConfig = std::get<0>(_renderStep_entityStandard.renderPassConfigs);

		_alphaSorter->waitThen([this, &renderPassConfig, &cameraData](const std::vector<Entity_alpha_sorter_entry>& entries) {
			auto& d3DDeviceResources = deviceResources();
			if (!_fatalError)
			{
				d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityStandard_draw");

				_cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
					for (const auto& entry : entities) {
						// TODO: Stats
						// ++_renderStats.alphaEntityCount;
						renderEntity(entry.entity, cameraData, _simpleLightProvider, renderPassConfig);
					}
				});

				d3DDeviceResources.PIXEndEvent();
			}
		});
	});

	_renderStep_debugElements.renderPasses[0] = std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
		_scene.manager<IDev_tools_manager>().renderDebugShapes(cameraData);
	});

	_renderStep_skybox.renderPasses[0] = std::make_unique<Render_pass_skybox>(_scene);
}

void Render_step_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{
	// Shadow map step resources
	_renderStep_shadowMap.data = std::make_shared<decltype(_renderStep_shadowMap.data)::element_type>();

	// Deferred lighting step resources
	_renderStep_entityDeferred.data = std::make_shared<decltype(_renderStep_entityDeferred.data)::element_type>();
	
	std::shared_ptr<Deferred_light_material> deferredLightMaterial;
	deferredLightMaterial.reset(new Deferred_light_material());

	_renderStep_entityDeferred.data->deferredLightMaterial = deferredLightMaterial;

	const auto& entityRenderManager = _scene.manager<IEntity_render_manager>();
	_renderStep_entityDeferred.data->pass0ScreenSpaceQuad = entityRenderManager.createScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
	_renderStep_entityDeferred.data->pass2ScreenSpaceQuad = entityRenderManager.createScreenSpaceQuad(deferredLightMaterial);

	// Debug elements lighting step resources
	_renderStep_debugElements.data = std::make_shared<decltype(_renderStep_debugElements.data)::element_type>();
	
	createRenderStepResources(_renderStep_shadowMap);
	createRenderStepResources(_renderStep_entityDeferred);
	createRenderStepResources(_renderStep_entityStandard);
	createRenderStepResources(_renderStep_debugElements);
	createRenderStepResources(_renderStep_skybox);
}

void Render_step_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND /*window*/, int width, int height)
{
	auto& d3DDeviceResources = deviceResources();
	const auto d3dDevice = d3DDeviceResources.GetD3DDevice();

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
		const auto depthTexture = std::make_shared<Depth_texture>(d3DDeviceResources);

		// Assign textures to the deferred light material
		auto deferredLightMaterial = _renderStep_entityDeferred.data->deferredLightMaterial;
		deferredLightMaterial->setColor0Texture(renderTargets0.at(0));
		deferredLightMaterial->setColor1Texture(renderTargets0.at(1));
		deferredLightMaterial->setColor2Texture(renderTargets0.at(2));
		deferredLightMaterial->setDepthTexture(depthTexture);
		deferredLightMaterial->setShadowMapDepthTexture(_scene.manager<IShadowmap_manager>().shadowMapDepthTextureArray());
		deferredLightMaterial->setShadowMapStencilTexture(_scene.manager<IShadowmap_manager>().shadowMapStencilTextureArray());

		// Give the render targets to the render pass
		std::get<0>(_renderStep_entityDeferred.renderPasses)->setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
		std::get<1>(_renderStep_entityDeferred.renderPasses)->setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
	}

	// Step Deferred, Pass 2
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
		renderTargets.resize(g_max_render_target_views, nullptr);

		renderTargets[0] = std::make_shared<Render_target_view_texture>(d3DDeviceResources.GetRenderTargetView());
		std::get<2>(_renderStep_entityDeferred.renderPasses)->setRenderTargets(std::move(renderTargets));
	}

	// Step Standard, Pass 0
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
		renderTargets.resize(g_max_render_target_views, nullptr);

		renderTargets[0] = std::make_shared<Render_target_view_texture>(d3DDeviceResources.GetRenderTargetView());
		std::get<0>(_renderStep_entityStandard.renderPasses)->setRenderTargets(std::move(renderTargets));
	}
}

void Render_step_manager::destroyDeviceDependentResources()
{
	// Unload shadow maps
	auto& shadowmapManager = _scene.manager<IShadowmap_manager>();
	for (const auto& lightEntity : *_lightEntities) {
		// Directional light only, right now
		const auto component = lightEntity->getFirstComponentOfType<Directional_light_component>();
		if (component && component->shadowsEnabled()) {
			if (component->shadowData())
				shadowmapManager.returnTexture(std::move(component->shadowData()));
			else
				assert(!component->shadowData());
		}
	}

	destroyRenderStepResources(_renderStep_shadowMap);
	destroyRenderStepResources(_renderStep_entityDeferred);
	destroyRenderStepResources(_renderStep_entityStandard);
	destroyRenderStepResources(_renderStep_debugElements);
	destroyRenderStepResources(_renderStep_skybox);

	_renderStep_shadowMap.data.reset();
	_renderStep_entityDeferred.data.reset();
	_renderStep_entityStandard.data.reset();
	_renderStep_debugElements.data.reset();
	_renderStep_skybox.data.reset();

	_fatalError = false;
}

void Render_step_manager::destroyWindowSizeDependentResources()
{
	std::get<0>(_renderStep_shadowMap.renderPasses)->clearRenderTargets();
	std::get<0>(_renderStep_entityDeferred.renderPasses)->clearRenderTargets();
	std::get<1>(_renderStep_entityDeferred.renderPasses)->clearRenderTargets();
	std::get<2>(_renderStep_entityDeferred.renderPasses)->clearRenderTargets();
	std::get<0>(_renderStep_entityStandard.renderPasses)->clearRenderTargets();
	std::get<0>(_renderStep_debugElements.renderPasses)->clearRenderTargets();
	std::get<0>(_renderStep_skybox.renderPasses)->clearRenderTargets();
}

void Render_step_manager::clearDepthStencil(float depth, uint8_t stencil) const
{
	auto& d3DDeviceResources = deviceResources();
	const auto depthStencil = d3DDeviceResources.GetDepthStencilView();
	d3DDeviceResources.GetD3DDeviceContext()->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void Render_step_manager::render(std::shared_ptr<Entity> cameraEntity)
{
	const auto cameraComponent = cameraEntity->getFirstComponentOfType<Camera_component>();
	if (!cameraComponent) {
		throw std::logic_error("Camera entity must have a Camera_component");
	}

	if (_fatalError)
		return;

	const auto frustum = _scene.manager<IEntity_render_manager>().createFrustum(
		*cameraComponent
	);

	_cullSorter->beginSortAsync(_renderableEntities->begin(), _renderableEntities->end(), frustum);

	// Create a camera matrix
	const auto cameraData = createCameraData(*cameraComponent);

	// Block on the cull sorter, since we can't render until it is done; and it is a good place to kick off the alpha sort.
	_cullSorter->waitThen([this, cameraEntity, cameraData](const std::vector<Entity_cull_sorter_entry>& entities) {
		// Find the list of entities that have alpha, and sort them.
		_alphaSorter->beginSortAsync(entities.begin(), entities.end(), cameraEntity->worldPosition());

		// Render steps
		_scene.manager<IEntity_render_manager>().clearRenderStats();

		clearDepthStencil();
		renderStep(_renderStep_shadowMap, cameraData);
		renderStep(_renderStep_entityDeferred, cameraData);
		renderStep(_renderStep_entityStandard, cameraData);
		renderStep(_renderStep_debugElements, cameraData);
		renderStep(_renderStep_skybox, cameraData);
	});
}

// TODO: Move this to a Render_pass_deferred class?
void Render_step_manager::renderLights(const Render_pass::Camera_data& cameraData, Render_pass_blend_mode blendMode)
{
	try
	{
		auto& quad = _renderStep_entityDeferred.data->pass2ScreenSpaceQuad;
		auto& entityRenderManager = _scene.manager<IEntity_render_manager>();

		const auto deferredLightMaterial = _renderStep_entityDeferred.data->deferredLightMaterial;
		assert(deferredLightMaterial == quad.material);

		std::vector<Entity*> deferredLights;
		const auto deferredLightProvider = [&deferredLights](const BoundingSphere& target, std::vector<Entity*>& lights, uint32_t maxLights) {
			if (lights.size() + deferredLights.size() > static_cast<size_t>(maxLights)) {
				throw std::logic_error("destination lights array is not large enough to contain Deferred_light_material's lights");
			}

			lights.insert(lights.end(), deferredLights.begin(), deferredLights.end());
		};

		auto lightIndex = 0;
		const auto maxLights = deferredLightMaterial->max_lights;
		auto renderedOnce = false;

		deferredLightMaterial->setupEmitted(true);
		for (auto eIter = _lightEntities->begin(); eIter != _lightEntities->end(); ++eIter, ++lightIndex) {
			deferredLights.push_back(eIter->get());

			if (lightIndex == maxLights) {
				entityRenderManager.renderRenderable(
					quad,
					Matrix::Identity,
					0.0f,
					cameraData,
					deferredLightProvider,
					blendMode,
					false);

				deferredLightMaterial->setupEmitted(false);
				renderedOnce = true;
				deferredLights.clear();

				lightIndex = 0;
			}
		}

		if (!deferredLights.empty() || !renderedOnce) {
			entityRenderManager.renderRenderable(
				quad,
				Matrix::Identity,
				0.0f,
				cameraData,
				deferredLightProvider,
				blendMode,
				false);
		}
	}
	catch (const std::runtime_error& e)
	{
		_fatalError = true;
		LOG(WARNING) << "Failed to render lights.\n" << e.what();
	}
}

Render_pass::Camera_data Render_step_manager::createCameraData(Camera_component& component)
{
	const auto viewport = deviceResources().GetScreenViewport();
	const auto aspectRatio = viewport.Width / viewport.Height;


	// Construct camera axis vectors, to create a view matrix using lookAt.
	const auto wt = component.entity().worldTransform();
	const auto pos = wt.Translation();
	const auto forward = Vector3::TransformNormal(Vector3::Forward, wt);
	const auto up = Vector3::TransformNormal(Vector3::Up, wt);

	// This optimization, while fancy, breaks our ability to read in the world pos from the depth buffer in deferred lighting.
	//auto invFarPlane = component.farPlane() != 0.0f ? 1.0f / component.farPlane() : 0.0f;
	//_cameraData.projectionMatrix._33 *= invFarPlane;
	//_cameraData.projectionMatrix._43 *= invFarPlane;

	return {
		Matrix::CreateLookAt(pos, pos + forward, up),
		Matrix::CreatePerspectiveFieldOfView(
			component.fov(),
			aspectRatio,
			component.nearPlane(),
			component.farPlane()),
		component.fov(),
		aspectRatio
	};
}

template<int TRender_pass_idx, class TData, class... TRender_passes>
void Render_step_manager::createRenderStepResources(Render_step<TData, TRender_passes...>& step)
{
	auto passConfig = std::get<TRender_pass_idx>(step.renderPassConfigs);
	auto& pass = *std::get<TRender_pass_idx>(step.renderPasses);

	auto& d3DResourcesManager = _scene.manager<ID3D_resources_manager>();
	auto& commonStates = d3DResourcesManager.commonStates();

	// Blend state
	if constexpr (passConfig.blendMode() == Render_pass_blend_mode::Opaque)
		pass.setBlendState(commonStates.Opaque());
	else if constexpr (passConfig.blendMode() == Render_pass_blend_mode::Blended_alpha)
		pass.setBlendState(commonStates.AlphaBlend());
	else if constexpr (passConfig.blendMode() == Render_pass_blend_mode::Additive)
		pass.setBlendState(commonStates.Additive());

	// Depth/Stencil
	if constexpr (Render_pass_stencil_mode::Disabled == passConfig.stencilMode()) {
		if constexpr (Render_pass_depth_mode::ReadWrite == passConfig.depthMode())
			pass.setDepthStencilState(commonStates.DepthDefault());
		else if constexpr (Render_pass_depth_mode::ReadOnly == passConfig.depthMode())
			pass.setDepthStencilState(commonStates.DepthRead());
		else
			pass.setDepthStencilState(commonStates.DepthNone());
	}
	else {
		D3D11_DEPTH_STENCIL_DESC desc;

		desc.DepthEnable = passConfig.depthMode() != Render_pass_depth_mode::Disabled ? TRUE : FALSE;
		desc.DepthWriteMask = passConfig.depthMode() == Render_pass_depth_mode::ReadWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		desc.StencilEnable = TRUE;
		desc.StencilReadMask = passConfig.stencilReadMask();
		desc.StencilWriteMask = passConfig.stencilWriteMask();

		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

		desc.BackFace = desc.FrontFace;

		ID3D11DepthStencilState* pResult;
		ThrowIfFailed(d3DResourcesManager.deviceResources().GetD3DDevice()->CreateDepthStencilState(&desc, &pResult));
		
		SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilEnabled");
		pass.setDepthStencilState(pResult);
		pResult->Release();
	}

	pass.createDeviceDependentResources();

	if constexpr (TRender_pass_idx + 1 < sizeof...(TRender_passes)) {
		createRenderStepResources<TRender_pass_idx + 1, TData, TRender_passes...>(step);
	}
}

template<int TRender_pass_idx, class TData, class... TRender_passes>
void Render_step_manager::destroyRenderStepResources(Render_step<TData, TRender_passes...>& step)
{
	auto& pass = std::get<TRender_pass_idx>(step.renderPasses);
	pass->setBlendState(nullptr);
	pass->setDepthStencilState(nullptr);
	pass->destroyDeviceDependentResources();

	if constexpr (TRender_pass_idx + 1 < sizeof...(TRender_passes)) {
		destroyRenderStepResources<TRender_pass_idx + 1, TData, TRender_passes...>(step);
	}
}

template<int TIdx, class TData, class... TRender_passes>
void Render_step_manager::renderStep(Render_step<TData, TRender_passes...>& step,
	const Render_pass::Camera_data& cameraData)
{
	if (!step.enabled)
		return;

	auto passConfig = std::get<TIdx>(step.renderPassConfigs);
	auto& pass = *std::get<TIdx>(step.renderPasses);

	auto& d3DDeviceResources = deviceResources();
	auto[renderTargetViews, numRenderTargets] = pass.renderTargetViewArray();
	if (numRenderTargets) {
		auto context = d3DDeviceResources.GetD3DDeviceContext();

		// TODO: Fix me!!!

		if constexpr (Render_pass_depth_mode::Disabled == passConfig.depthMode())
			context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, nullptr);
		else
			context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, d3DDeviceResources.GetDepthStencilView());
	}

	renderPass(passConfig, pass, cameraData);

	if constexpr (TIdx + 1 < sizeof...(TRender_passes)) {
		renderStep<TIdx + 1, TData, TRender_passes...>(step, cameraData);
	}
}

template<
	Render_pass_blend_mode TBlend_mode,
	Render_pass_depth_mode TDepth_mode,
	Render_pass_stencil_mode TStencil_mode,
	uint32_t TStencil_read_mask,
	uint32_t TStencil_write_mask
>
void Render_step_manager::renderPass(Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask, TStencil_write_mask>& renderPassInfo,
	Render_pass& renderPass,
	const Render_pass::Camera_data& cameraData)
{
	auto context = deviceResources().GetD3DDeviceContext();
	auto& commonStates = _scene.manager<ID3D_resources_manager>().commonStates();

	// Set the blend mode
	constexpr auto opaqueSampleMask = 0xffffffff;
	constexpr std::array<float, 4> opaqueBlendFactor{ 0.0f, 0.0f, 0.0f, 0.0f };

	context->OMSetBlendState(renderPass.blendState(), opaqueBlendFactor.data(), opaqueSampleMask);
	
	// Depth/Stencil buffer mode
	context->OMSetDepthStencilState(renderPass.depthStencilState(), renderPass.stencilRef());

	// Make sure wire-frame is disabled
	context->RSSetState(commonStates.CullClockwise());

	// Set the viewport.
	auto& d3DDeviceResources = deviceResources();
	auto viewport = d3DDeviceResources.GetScreenViewport();
	d3DDeviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);

	// Call the render method.
	renderPass.render(cameraData);
}

template<
	Render_pass_blend_mode TBlend_mode,
	Render_pass_depth_mode TDepth_mode,
	Render_pass_stencil_mode TStencil_mode,
	uint32_t TStencilReadMask,
	uint32_t TStencilWriteMask
>
void Render_step_manager::renderEntity(Entity* entity,
	const Render_pass::Camera_data& cameraData,
	Light_provider::Callback_type& lightProvider,
	const Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencilReadMask, TStencilWriteMask>& renderPassInfo)
{
	const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
	if (!renderable || !renderable->visible())
		return;

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

	_scene.manager<IEntity_render_manager>().renderEntity(*renderable, cameraData, lightProvider, TBlend_mode);
}
