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
#include "Renderable_component.h"
#include "Deferred_light_material.h"
#include "TextureImpl.h"
#include "Camera_component.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

// Static initialization order means we can't use Matrix::Identity here.
const Entity_render_manager::Camera_data Entity_render_manager::Camera_data::identity = { 
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f }
};

Entity_render_manager::Renderable::Renderable()
	: meshData(nullptr)
	, material(nullptr)
	, rendererData(nullptr)
{
}

Entity_render_manager::Entity_render_manager(Scene& scene, std::shared_ptr<Material_repository> materialRepository, DX::DeviceResources& deviceResources)
	: Manager_base(scene)
	, _deviceResources(deviceResources)
	, _renderableEntities(nullptr)
	, _materialRepository(materialRepository)
	, _primitiveMeshDataFactory(nullptr)
	, _fatalError(false)
	, _rasterizerStateDepthDisabled(nullptr)
	, _rasterizerStateDepthEnabled(nullptr)
{
}

Entity_render_manager::~Entity_render_manager()
{
}

void Entity_render_manager::initialize()
{
	_renderableEntities = _scene.sceneGraphManager().getEntityFilter({ Renderable_component::type() });
	_lightEntities = _scene.sceneGraphManager().getEntityFilter({ 
		Directional_light_component::type(),
		Point_light_component::type(),
		Ambient_light_component::type()
	}, Entity_filter_mode::Any);

	_primitiveMeshDataFactory = std::make_unique<Primitive_mesh_data_factory>();
}

void Entity_render_manager::tick() 
{
	const auto viewport = _deviceResources.GetScreenViewport();
	const float aspectRatio = viewport.Width / viewport.Height;
	float invFarPlane;

	const std::shared_ptr<Entity> cameraEntity = _scene.mainCamera();
	if (cameraEntity)
	{
		// TODO: Replace this with a 'scriptable' component on the camera
		//cameraEntity->LookAt({ 5, 0, 5 }, Vector3::Up);
		Camera_component* component = cameraEntity->getFirstComponentOfType<Camera_component>();
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
		constexpr float defaultFarPlane = 30.0f;
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

	_rasterizerStateDepthDisabled.Reset();
	_rasterizerStateDepthEnabled.Reset();

	_pass1ScreenSpaceQuad = Renderable();
	_pass2ScreenSpaceQuad = Renderable();
	
	_depthTexture.reset();
	_pass1RenderTargets.clear();
}

void Entity_render_manager::createDeviceDependentResources()
{
	_rasterizerStateDepthDisabled.Reset();
	_rasterizerStateDepthEnabled.Reset();

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthClipEnable = false;
	ThrowIfFailed(_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, _rasterizerStateDepthDisabled.ReleaseAndGetAddressOf()),
		"Create rasterizerStateDepthDisabled");

	// Render using a right handed coordinate system
	rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.FrontCounterClockwise = true;
	ThrowIfFailed(_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, _rasterizerStateDepthEnabled.ReleaseAndGetAddressOf()),
		"Create rasterizerStateDepthEnabled");

	// Depth buffer settings
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ThrowIfFailed(_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, _depthStencilStateDepthDisabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthDisabled");

	depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	ThrowIfFailed(_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, _depthStencilStateDepthEnabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthEnabled");

	// Initial settings
	setDepthEnabled(false);

	// Deffered Lighting Material
	_deferredLightMaterial = std::make_shared<Deferred_light_material>();

	// Full-screen quads
	_pass1ScreenSpaceQuad = initScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
	_pass2ScreenSpaceQuad = initScreenSpaceQuad(_deferredLightMaterial);
}

void Entity_render_manager::createWindowSizeDependentResources()
{
	assert(_pass1RenderTargets.empty());
	const auto d3dDevice = _deviceResources.GetD3DDevice();

	// Determine the render target size in pixels.
	const auto& outputSize = _deviceResources.GetOutputSize();
	const UINT width = std::max<UINT>(outputSize.right - outputSize.left, 1);
	const UINT height = std::max<UINT>(outputSize.bottom - outputSize.top, 1);
	
	for (int i = 0; i < 3; ++i)
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
	_rasterizerStateDepthDisabled.Reset();
	_rasterizerStateDepthEnabled.Reset();
	
	_pass1ScreenSpaceQuad.rendererData.reset();
	_pass2ScreenSpaceQuad.rendererData.reset();
	
	_deferredLightMaterial.reset();	
	_fatalError = false;
}

void oe::Entity_render_manager::destroyWindowSizeDependentResources()
{
	_depthTexture.reset();
	_pass1RenderTargets.clear();
}

void Entity_render_manager::render()
{
	if (!_fatalError)
	{
		_deviceResources.PIXBeginEvent(L"setupPass1");
		setupRenderEntities();
		_deviceResources.PIXEndEvent();

		_deviceResources.PIXBeginEvent(L"renderEntities");
		renderEntities();
		_deviceResources.PIXEndEvent();
	}

	_deviceResources.PIXBeginEvent(L"setupPass2");
	setupRenderLights();
	_deviceResources.PIXEndEvent();

	if (!_fatalError)
	{
		_deviceResources.PIXBeginEvent(L"renderLights");
		renderLights();
		_deviceResources.PIXEndEvent();
	}
}

void Entity_render_manager::renderEntities()
{
	// Arrays defined outside of the loop so that their memory is re-used.
	Buffer_array_set bufferArraySet;

	std::vector<Vertex_attribute> vertexAttributes;

	// Iterate over all entities which have a RenderableComponent
	for (auto iter = _renderableEntities->begin(); iter != _renderableEntities->end(); ++iter)
	{
		const auto entity = *iter;
		auto renderable = entity->getFirstComponentOfType<Renderable_component>();
		if (!renderable->visible())
			continue;

		try {
			const auto material = renderable->material().get();
			assert(material != nullptr);

			// TODO: Support alpha in a second pass
			if (material->getAlphaMode() != Material_alpha_mode::Opaque)
				continue;

			const Renderer_data* rendererData = renderable->rendererData().get();
			if (rendererData == nullptr) {
				
				const auto meshDataComponent = entity->getFirstComponentOfType<Mesh_data_component>();
				if (meshDataComponent == nullptr || meshDataComponent->meshData() == nullptr)
				{
					// There is no mesh data, we can't render.
					continue;
				}
				const auto& meshData = meshDataComponent->meshData();

				LOG(INFO) << "Creating renderer data for entity " << entity->getName() << " (ID " << entity->getId() << ")";

				vertexAttributes.clear();
				material->vertexAttributes(vertexAttributes);

				auto rendererDataPtr = createRendererData(*meshData, vertexAttributes);
				rendererData = rendererDataPtr.get();
				renderable->setRendererData(std::move(rendererDataPtr));
			}

			drawRendererData(_cameraData, entity->worldTransform(), rendererData, material, bufferArraySet);
		} 
		catch (std::runtime_error& e)
		{
			renderable->setVisible(false);
			LOG(WARNING) << "Failed to render mesh on entity " << entity->getName() << " (ID " << entity->getId() << "): " << e.what();
		}
	}
}

void Entity_render_manager::renderLights()
{
	const auto identity = XMMatrixIdentity();
	Buffer_array_set bufferArraySet;

	try
	{
		const auto rendererData = _pass2ScreenSpaceQuad.rendererData.get();
		if (!rendererData || !rendererData->vertexCount)
			return;

		loadRendererDataToDeviceContext(rendererData, bufferArraySet);
		for (auto eIter = _lightEntities->begin(); eIter != _lightEntities->end(); ++eIter)
		{
			const auto lightEntity = *eIter;
			bool foundLight = false;
			
			const auto directionalLight = lightEntity->getFirstComponentOfType<Directional_light_component>();
			if (directionalLight)
			{
				const auto lightDirection = Vector3::Transform(Vector3::Forward, lightEntity->worldRotation());
				_deferredLightMaterial->setupDirectionalLight(lightDirection, directionalLight->color(), directionalLight->intensity());
				foundLight = true;
			}
			
			const auto pointLight = lightEntity->getFirstComponentOfType<Point_light_component>();
			if (pointLight)
			{
				_deferredLightMaterial->setupPointLight(lightEntity->worldPosition(), pointLight->color(), pointLight->intensity());
				foundLight = true;
			}

			const auto ambientLight = lightEntity->getFirstComponentOfType<Ambient_light_component>();
			if (ambientLight)
			{
				_deferredLightMaterial->setupAmbientLight(ambientLight->color(), ambientLight->intensity());
				foundLight = true;
			}

			if (foundLight)
			{
				if (_deferredLightMaterial->render(*rendererData, _cameraData.worldMatrix, _cameraData.viewMatrix, _cameraData.projectionMatrix, _deviceResources))
					_deviceResources.GetD3DDeviceContext()->DrawIndexed(rendererData->indexCount, 0, 0);
			}
		}

		// Finally, render emitted light sources
		_deferredLightMaterial->setupEmitted();
		if (_deferredLightMaterial->render(*rendererData, _cameraData.worldMatrix, _cameraData.viewMatrix, _cameraData.projectionMatrix, _deviceResources))
			_deviceResources.GetD3DDeviceContext()->DrawIndexed(rendererData->indexCount, 0, 0);

		_deferredLightMaterial->unbind(_deviceResources);
	}
	catch (std::runtime_error& e)
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
		deviceContext->RSSetState(_rasterizerStateDepthEnabled.Get());
		deviceContext->OMSetDepthStencilState(_depthStencilStateDepthEnabled.Get(), 0);
	}
	else
	{
		deviceContext->RSSetState(_rasterizerStateDepthDisabled.Get());
		deviceContext->OMSetDepthStencilState(_depthStencilStateDepthDisabled.Get(), 0);
	}
}

void Entity_render_manager::loadRendererDataToDeviceContext(const Renderer_data* rendererData, Buffer_array_set& bufferArraySet) const
{
	const auto deviceContext = _deviceResources.GetD3DDeviceContext();

	// Send the buffers to the input assembler
	if (!rendererData->vertexBuffers.empty()) {
		auto& vertexBuffers = rendererData->vertexBuffers;
		const auto numBuffers = vertexBuffers.size();
		if (numBuffers > 1) {
			bufferArraySet.bufferArray.clear();
			bufferArraySet.strideArray.clear();
			bufferArraySet.offsetArray.clear();

			for (std::vector<Mesh_buffer_accessor>::size_type i = 0; i < numBuffers; ++i)
			{
				const D3D_buffer_accessor* vertexBufferDesc = rendererData->vertexBuffers[i].get();
				bufferArraySet.bufferArray.push_back(vertexBufferDesc->buffer->d3dBuffer);
				bufferArraySet.strideArray.push_back(vertexBufferDesc->stride);
				bufferArraySet.offsetArray.push_back(vertexBufferDesc->offset);
			}

			deviceContext->IASetVertexBuffers(0, static_cast<UINT>(numBuffers), bufferArraySet.bufferArray.data(), bufferArraySet.strideArray.data(), bufferArraySet.offsetArray.data());
		}
		else
		{
			const D3D_buffer_accessor* vertexBufferDesc = rendererData->vertexBuffers[0].get();
			deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDesc->buffer->d3dBuffer, &vertexBufferDesc->stride, &vertexBufferDesc->offset);
		}

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		deviceContext->IASetPrimitiveTopology(rendererData->topology);

		if (rendererData->indexBufferAccessor != nullptr)
		{
			// Set the index buffer to active in the input assembler so it can be rendered.
			const auto indexAccessor = rendererData->indexBufferAccessor.get();
			deviceContext->IASetIndexBuffer(indexAccessor->buffer->d3dBuffer, rendererData->indexFormat, indexAccessor->offset);
		}
	}
}

void Entity_render_manager::drawRendererData(
	const Camera_data& cameraData,
	const Matrix& worldTransform,
	const Renderer_data* rendererData,
	Material* material,
	Buffer_array_set& bufferArraySet) const
{
	if (rendererData->vertexBuffers.empty())
		return;

	const auto deviceContext = _deviceResources.GetD3DDeviceContext();
	loadRendererDataToDeviceContext(rendererData, bufferArraySet);

	if (rendererData->indexBufferAccessor != nullptr)
	{
		if (material->render(*rendererData, worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, _deviceResources))
			deviceContext->DrawIndexed(rendererData->indexCount, 0, 0);
	}
	else
	{
		if (material->render(*rendererData, worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, _deviceResources))
		{
			// Render the triangles.
			deviceContext->Draw(rendererData->vertexCount, rendererData->vertexCount);
		}
	}
}


std::unique_ptr<Renderer_data> Entity_render_manager::createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const
{
	auto rendererData = std::make_unique<Renderer_data>();
	rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

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
		renderable.meshData = _primitiveMeshDataFactory->createQuad(Vector2(2.0f, 2.0f), Vector3(-1.f, -1.f, 0.f));

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

void Entity_render_manager::setupRenderEntities()
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
	if (_pass1ScreenSpaceQuad.rendererData && _pass1ScreenSpaceQuad.material) {
		const auto identity = XMMatrixIdentity();
		Buffer_array_set bufferArraySet;

		try
		{
			drawRendererData(Camera_data::identity, identity, _pass1ScreenSpaceQuad.rendererData.get(), _pass1ScreenSpaceQuad.material.get(), bufferArraySet);
		}
		catch (std::runtime_error& e)
		{
			_fatalError = true;
			LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
		}
	}

	setDepthEnabled(true);
}

void Entity_render_manager::setupRenderLights()
{
	// Clear the views.
	auto context = _deviceResources.GetD3DDeviceContext();
	
	context->ClearRenderTargetView(_deviceResources.GetRenderTargetView(), Colors::Black);

	const auto finalColorRenderTarget = _deviceResources.GetRenderTargetView();
	
	setDepthEnabled(false);
	context->OMSetRenderTargets(1, &finalColorRenderTarget, nullptr);
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
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = buffer.data;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Get a reference
	HRESULT hr = _deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &d3dBuffer->d3dBuffer);
	assert(!FAILED(hr));

	return d3dBuffer;
}

std::unique_ptr<Material> Entity_render_manager::loadMaterial(const std::string& materialName) const
{
	return _materialRepository->instantiate(materialName);
}
