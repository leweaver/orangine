#include "pch.h"
#include "Entity_render_manager.h"
#include "Scene_graph_manager.h"
#include "Renderer_data.h"
#include "Material.h"
#include "Entity.h"
#include "Entity_filter.h"
#include "Scene.h"

#include <set>
#include <array>
#include <functional>
#include "Light_component.h"
#include "Clear_gbuffer_material.h"
#include "Render_target_texture.h"
#include "Render_light_data.h"
#include "Renderable_component.h"
#include "Deferred_light_material.h"
#include "Texture.h"
#include "Camera_component.h"
#include "Entity_sorter.h"
#include "CommonStates.h"
#include "GeometricPrimitive.h"
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

Entity_render_manager::Renderable::Renderable()
	: meshData(nullptr)
	, material(nullptr)
	, rendererData(nullptr)
{
}

Entity_render_manager::Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository, DX::DeviceResources& deviceResources)
	: IEntity_render_manager(scene)
	, _deviceResources(deviceResources)
	, _renderableEntities(nullptr)
	, _materialRepository(move(materialRepository))
	, _primitiveMeshDataFactory(nullptr)
	, _enableDeferredRendering(true)
	, _fatalError(false)
	, _lastBlendEnabled(false)
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
	
	_pass1ScreenSpaceQuad = Renderable();
	_pass2ScreenSpaceQuad = Renderable();
	
	_depthTexture.reset();
	_pass1RenderTargets.clear();

	auto context = _deviceResources.GetD3DDeviceContext();
	ID3D11RenderTargetView* renderTargetViews[] = { nullptr, nullptr, nullptr };
	context->OMSetRenderTargets(3, renderTargetViews, nullptr);
}

void Entity_render_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{	
	auto device = _deviceResources.GetD3DDevice();
	_commonStates = std::make_unique<CommonStates>(device);

	// Depth buffer settings
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ThrowIfFailed(device->CreateDepthStencilState(&depthStencilDesc, _depthStencilStateDepthDisabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthDisabled");

	depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	ThrowIfFailed(device->CreateDepthStencilState(&depthStencilDesc, _depthStencilStateDepthEnabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthEnabled");

	// Initial settings
	setDepthEnabled(false);

	// Deffered Lighting Material
	_deferredLightMaterial = std::shared_ptr<Deferred_light_material>(new Deferred_light_material());
	
	// additive blend
	D3D11_BLEND_DESC blendStateDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendStateDesc.RenderTarget[0].BlendEnable = true;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	device->CreateBlendState(&blendStateDesc, &_deferredLightBlendState);

	// Full-screen quads
	_pass1ScreenSpaceQuad = initScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
	_pass2ScreenSpaceQuad = initScreenSpaceQuad(_deferredLightMaterial);

	_renderPass_entityDeferred_renderLightData = std::make_unique<decltype(_renderPass_entityDeferred_renderLightData)::element_type>(device);
	_renderPass_entityDeferred_renderLightData_blank = std::make_unique<decltype(_renderPass_entityDeferred_renderLightData_blank)::element_type>(device);
	_renderPass_entityStandard_renderLightData = std::make_unique<decltype(_renderPass_entityStandard_renderLightData)::element_type>(device);

	// Debug meshes
/*	_simplePrimitives.push_back(
		GeometricPrimitive::CreateTeapot(context)
	);*/
}

void Entity_render_manager::createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND /*window*/, int width, int height)
{
	assert(_pass1RenderTargets.empty());
	const auto d3dDevice = _deviceResources.GetD3DDevice();

	// Determine the render target size in pixels.	
	for (auto i = 0; i < 3; ++i)
	{
		auto target = std::make_unique<Render_target_texture>(width, height);
		target->load(d3dDevice);
		_pass1RenderTargets.push_back(move(target));
	}

	// Create a depth texture resource
	_depthTexture = std::make_unique<Depth_texture>(_deviceResources);

	// Assign textures to the deferred light material
	_deferredLightMaterial->setColor0Texture(_pass1RenderTargets.at(0));
	_deferredLightMaterial->setColor1Texture(_pass1RenderTargets.at(1));
	_deferredLightMaterial->setColor2Texture(_pass1RenderTargets.at(2));
	_deferredLightMaterial->setDepthTexture(_depthTexture);

	// Set the viewport.
	auto viewport = _deviceResources.GetScreenViewport();
	_deviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);
}

void Entity_render_manager::destroyDeviceDependentResources()
{
	_commonStates.reset();

	_depthStencilStateDepthDisabled.Reset();
	_depthStencilStateDepthEnabled.Reset();
	
	_pass1ScreenSpaceQuad.rendererData.reset();
	_pass2ScreenSpaceQuad.rendererData.reset();
	
	_deferredLightMaterial.reset();	
	_deferredLightBlendState.Reset();
	
	_renderPass_entityStandard_renderLightData.reset();
	_renderPass_entityDeferred_renderLightData.reset();
	_renderPass_entityDeferred_renderLightData_blank.reset();

	_fatalError = false;
}

void Entity_render_manager::destroyWindowSizeDependentResources()
{
	_depthTexture.reset();
	_pass1RenderTargets.clear();
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

void Entity_render_manager::render()
{
	const auto cameraEntity = _scene.mainCamera();
	if (!cameraEntity)
		return;

	if (_fatalError)
		return;

	Buffer_array_set bufferArraySet;
	
	const auto frustum = createFrustum(*cameraEntity.get(), *cameraEntity->getFirstComponentOfType<Camera_component>());
	_cullSorter->beginSortAsync(_renderableEntities->begin(), _renderableEntities->end(), frustum);

	// Block on the cull sorter, since we can't render until it is done; and it is a good place to kick off the alpha sort.
	_cullSorter->then([&](const std::vector<Entity_cull_sorter_entry>& entities) {
		// Find the list of entities that have alpha, and sort them.
		_alphaSorter->beginSortAsync(entities.begin(), entities.end(), cameraEntity->worldPosition());
	});
	
	// Deferred Lighting passes
	setBlendEnabled(false); 
	_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step1_setup");
	setupRenderPass_entityDeferred_step1();
	_deviceResources.PIXEndEvent();

	if (_enableDeferredRendering) {
		_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step1_draw");

		const auto lightDataProvider = [this](const Entity&) { return _renderPass_entityDeferred_renderLightData_blank.get(); };

		_cullSorter->then([&](const std::vector<Entity_cull_sorter_entry>& entities) {
			for (const auto& entry : entities)
				render(entry.entity, bufferArraySet, lightDataProvider, std::get<g_entity_deferred_geometry>(_renderPass_entityDeferred));
		});
		
		_deviceResources.PIXEndEvent();
	}

	_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_setup");
	setupRenderPass_entityDeferred_step2();
	_deviceResources.PIXEndEvent();

	if (!_fatalError)
	{
		if (_enableDeferredRendering) {
			_deviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_draw");
			renderLights();
			_deviceResources.PIXEndEvent();
		}
	}

	// Standard Lighting pass
	_alphaSorter->then([this, &bufferArraySet](const std::vector<Entity_alpha_sorter_entry>& entries) {
		if (!_fatalError)
		{
			_deviceResources.PIXBeginEvent(L"renderPass_EntityStandard_setup");
			setupRenderPass_entityStandard();
			_deviceResources.PIXEndEvent();

			const auto lightDataProvider = [this](const Entity&) {
				_renderPass_entityStandard_renderLightData->clear();
				for (auto iter = _lightEntities->begin(); iter != _lightEntities->end(); ++iter) {
					if (_renderPass_entityStandard_renderLightData->full())
						break;

					addLightToRenderLightData(*iter->get(), *_renderPass_entityStandard_renderLightData);
				}
				_renderPass_entityStandard_renderLightData->updateBuffer(_deviceResources.GetD3DDeviceContext());
				;			return _renderPass_entityStandard_renderLightData.get();
			};

			_deviceResources.PIXBeginEvent(L"renderPass_EntityStandard_draw");

			_cullSorter->then([&](const std::vector<Entity_cull_sorter_entry>& entities) {
				for (const auto& entry : entities)
					render(entry.entity, bufferArraySet, lightDataProvider, std::get<0>(_renderPass_entityStandard));
			});

			_deviceResources.PIXEndEvent();
		}
	});
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

	// TODO: For some reason, CreateFromMatrix sets near/far to -ve values?
	//frustum.Near = cameraComponent.nearPlane();
	//frustum.Far = cameraComponent.farPlane();

	return frustum;
}

std::vector<Vertex_attribute> vertexAttributes;

template<Render_pass_output_format TOutput_format>
void Entity_render_manager::render(Entity* entity,
	Buffer_array_set& bufferArraySet,
	const Light_data_provider& lightDataProvider,
	const Render_pass_info<TOutput_format>& renderPassInfo)
{
	auto renderable = entity->getFirstComponentOfType<Renderable_component>();
	if (!renderable->visible())
		return;

	try {
		const auto material = renderable->material().get();
		assert(material != nullptr);

		// Check that this render pass supports this materials alpha mode
		if constexpr (Render_pass_info<TOutput_format>::supportsBlendedAlpha()) {
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

			vertexAttributes.clear();
			material->vertexAttributes(vertexAttributes);

			auto rendererDataPtr = createRendererData(*meshData, vertexAttributes);
			rendererData = rendererDataPtr.get();
			renderable->setRendererData(std::move(rendererDataPtr));
		}

		const auto renderLightData = lightDataProvider(*entity);
		if (!renderLightData)
			throw std::runtime_error("Could not get Render_light_data");

		drawRendererData(
			_cameraData,
			entity->worldTransform(),
			*rendererData,
			renderPassInfo,
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
	Buffer_array_set bufferArraySet;
	auto context = _deviceResources.GetD3DDeviceContext();

	try
	{
		const auto rendererData = _pass2ScreenSpaceQuad.rendererData.get();
		if (!rendererData || !rendererData->vertexCount)
			return;

		const auto renderLight = [&]() {
			_renderPass_entityDeferred_renderLightData->updateBuffer(context);
			const auto renderSuccess = _deferredLightMaterial->render(
				*rendererData,
				std::get<g_entity_deferred_lights>(_renderPass_entityDeferred).outputFormat,
				*_renderPass_entityDeferred_renderLightData,
				_cameraData.worldMatrix,
				_cameraData.viewMatrix,
				_cameraData.projectionMatrix,
				_deviceResources);

			_renderPass_entityDeferred_renderLightData->clear();
		};

		loadRendererDataToDeviceContext(*rendererData, bufferArraySet);

		// Render emitted ight sources once only.
		_deferredLightMaterial->setupEmitted(true);
		auto renderedEmitted = false;

		// Render directional, point, ambient
		for (auto eIter = _lightEntities->begin(); eIter != _lightEntities->end(); ++eIter)
		{
			addLightToRenderLightData(*eIter->get(), *_renderPass_entityDeferred_renderLightData);
			
			if (_renderPass_entityDeferred_renderLightData->full()) {
				renderLight();
				renderedEmitted = true;
				_deferredLightMaterial->setupEmitted(false);
			}
		}

		if (!_renderPass_entityDeferred_renderLightData->empty() || !renderedEmitted)
			renderLight();

		// Unbind material
		_deferredLightMaterial->unbind(_deviceResources);
	}
	catch (const std::runtime_error& e)
	{
		_fatalError = true;
		LOG(WARNING) << "Failed to render lights.\n" << e.what();
	}
}

void Entity_render_manager::setDepthEnabled(bool enabled)
{
	const auto deviceContext = _deviceResources.GetD3DDeviceContext();
	if (enabled)
	{
		deviceContext->OMSetDepthStencilState(_depthStencilStateDepthEnabled.Get(), 0);
	}
	else
	{
		deviceContext->OMSetDepthStencilState(_depthStencilStateDepthDisabled.Get(), 0);
	}
}

void Entity_render_manager::setBlendEnabled(bool enabled)
{
	constexpr auto opaqueSampleMask = 0xffffffff;
	constexpr std::array<float, 4> opaqueBlendFactor{ 0.0f, 0.0f, 0.0f, 0.0f };
	
	const auto blendState = enabled ? _commonStates->AlphaBlend() : _commonStates->Opaque();
	_deviceResources.GetD3DDeviceContext()->OMSetBlendState(blendState, opaqueBlendFactor.data(), opaqueSampleMask);
	_lastBlendEnabled = enabled;
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

template<Render_pass_output_format TOutputFormat>
void Entity_render_manager::drawRendererData(
	const Camera_data& cameraData,
	const Matrix& worldTransform,
	const Renderer_data& rendererData,
	const Render_pass_info<TOutputFormat>& renderPassInfo,
	const Render_light_data& renderLightData,
	Material& material,
	bool wireframe,
	Buffer_array_set& bufferArraySet) const
{
	if (rendererData.vertexBuffers.empty())
		return;

	const auto deviceContext = _deviceResources.GetD3DDeviceContext();
	loadRendererDataToDeviceContext(rendererData, bufferArraySet);
	
	// Set the rasterizer state
	if (wireframe)
		deviceContext->RSSetState(_commonStates->Wireframe());
	else
		deviceContext->RSSetState(_commonStates->CullClockwise());

	// Render the trianges
	material.render(rendererData, renderPassInfo.outputFormat, renderLightData, 
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

void Entity_render_manager::setupRenderPass_entityDeferred_step1()
{
	// Clear the views.
	auto context = _deviceResources.GetD3DDeviceContext();

	const auto depthStencil = _deviceResources.GetDepthStencilView();
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	constexpr size_t numRenderTargets = 3;
	assert(_pass1RenderTargets.size() == numRenderTargets);
	std::array<ID3D11RenderTargetView*, numRenderTargets> renderTargets = {
		_pass1RenderTargets[0]->renderTargetView(),
		_pass1RenderTargets[1]->renderTargetView(),
		_pass1RenderTargets[2]->renderTargetView(),
	};

	context->OMSetRenderTargets(numRenderTargets, renderTargets.data(), depthStencil);
	
	// Clear the rendered textures (ignoring depth)
	setDepthEnabled(false);
	if (_pass1ScreenSpaceQuad.rendererData && _pass1ScreenSpaceQuad.material) {
		const auto identity = XMMatrixIdentity();
		Buffer_array_set bufferArraySet;

		try
		{
			drawRendererData(Camera_data::identity, 
				identity, 
				*_pass1ScreenSpaceQuad.rendererData, 
				std::get<g_entity_deferred_cleargbuffer>(_renderPass_entityDeferred),
				*_renderPass_entityDeferred_renderLightData_blank,
				*_pass1ScreenSpaceQuad.material, 
				false,
				bufferArraySet);
		}
		catch (std::runtime_error& e)
		{
			_fatalError = true;
			LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
		}
	}

	setDepthEnabled(true);

	// Make sure wireframe is disabled
	context->RSSetState(_commonStates->CullClockwise());
}

void Entity_render_manager::setupRenderPass_entityDeferred_step2()
{
	auto context = _deviceResources.GetD3DDeviceContext();

	// Clear the views.	
	context->ClearRenderTargetView(_deviceResources.GetRenderTargetView(), Colors::Black);

	ID3D11RenderTargetView* renderTargets[] = {
		_deviceResources.GetRenderTargetView(),
		nullptr,
		nullptr
	};
	
	setDepthEnabled(false);
	context->OMSetRenderTargets(3, renderTargets, nullptr);

	constexpr float blendFactor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(_deferredLightBlendState.Get(), blendFactor, 0xFFFFFFFF);

	// Make sure wireframe is disabled
	context->RSSetState(_commonStates->CullClockwise());

}

void Entity_render_manager::setupRenderPass_entityStandard()
{
	auto context = _deviceResources.GetD3DDeviceContext();
	const auto depthStencil = _deviceResources.GetDepthStencilView();

	const auto finalColorRenderTarget = _deviceResources.GetRenderTargetView();

	setBlendEnabled(true);
	setDepthEnabled(true);
	context->OMSetRenderTargets(1, &finalColorRenderTarget, depthStencil);
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

	// Fill in the subresource data.
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
