#include "pch.h"
#include "EntityRenderManager.h"
#include "SceneGraphManager.h"
#include "RendererData.h"
#include "Material.h"
#include "Entity.h"
#include "EntityFilter.h"
#include "Scene.h"

#include <set>
#include <functional>
#include "LightComponent.h"
#include "ClearGBufferMaterial.h"
#include "TextureRenderTarget.h"
#include "RenderableComponent.h"
#include "DeferredLightMaterial.h"
#include "TextureImpl.h"
#include "CameraComponent.h"

using namespace OE;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

// Static initialization order means we can't use Matrix::Identity here.
const EntityRenderManager::CameraData EntityRenderManager::CameraData::Identity = { 
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },
	{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f }
};

EntityRenderManager::Renderable::Renderable()
	: meshData(nullptr)
	, material(nullptr)
	, rendererData(nullptr)
{
}

EntityRenderManager::EntityRenderManager(Scene& scene, const std::shared_ptr<MaterialRepository> &materialRepository, DX::DeviceResources &deviceResources)
	: ManagerBase(scene)
	, m_deviceResources(deviceResources)
	, m_renderableEntities(nullptr)
	, m_materialRepository(materialRepository)
	, m_primitiveMeshDataFactory(nullptr)
	, m_fatalError(false)
	, m_rasterizerStateDepthDisabled(nullptr)
	, m_rasterizerStateDepthEnabled(nullptr)
{
}

EntityRenderManager::~EntityRenderManager()
{
}

void EntityRenderManager::Initialize()
{
	m_renderableEntities = m_scene.GetSceneGraphManager().GetEntityFilter({ RenderableComponent::Type() });
	m_lightEntities = m_scene.GetSceneGraphManager().GetEntityFilter({ 
		DirectionalLightComponent::Type(),
		PointLightComponent::Type()
	}, EntityFilterMode::ANY);

	m_primitiveMeshDataFactory = std::make_unique<PrimitiveMeshDataFactory>();
}

void EntityRenderManager::Tick() 
{
	const auto viewport = m_deviceResources.GetScreenViewport();
	const float aspectRatio = viewport.Width / viewport.Height;
	float invFarPlane;

	const std::shared_ptr<Entity> cameraEntity = m_scene.MainCamera();
	if (cameraEntity)
	{
		// TODO: Replace this with a 'scriptable' component on the camera
		//cameraEntity->LookAt({ 5, 0, 5 }, Vector3::Up);
		CameraComponent* component = cameraEntity->GetFirstComponentOfType<CameraComponent>();
		assert(!!component);

		auto projMatrix = Matrix::CreatePerspectiveFieldOfView(
			component->Fov(), 
			aspectRatio,
			component->NearPlane(), 
			component->FarPlane());

		if (component->FarPlane() == 0.0f)
			invFarPlane = 0.0f;
		else
			invFarPlane = 1.0f / component->FarPlane();

		// Construct camera axis vectors, to create a view matrix using lookAt.
		const auto wt = cameraEntity->WorldTransform();
		const auto pos = wt.Translation();
		const auto forward = Vector3::TransformNormal(Vector3::Forward, wt);
		const auto up = Vector3::TransformNormal(Vector3::Up, wt);

		auto viewMatrix = Matrix::CreateLookAt(pos, pos + forward, up);
		m_cameraData = {
			viewMatrix,
			projMatrix
		};
	}
	else
	{
		// Create a default camera
		constexpr float defaultFarPlane = 30.0f;
		m_cameraData = {
			Matrix::CreateLookAt({ 0.0f, 0.0f, 10.0f }, Vector3::Zero, Vector3::Up),
			Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(60.0f), aspectRatio, 0.1f, defaultFarPlane)
		};
		invFarPlane = 1.0f / defaultFarPlane;
	}

	m_cameraData.projectionMatrix._33 *= invFarPlane;
	m_cameraData.projectionMatrix._43 *= invFarPlane;
}

void EntityRenderManager::Shutdown()
{
	if (m_primitiveMeshDataFactory)
		m_primitiveMeshDataFactory.reset();

	m_rasterizerStateDepthDisabled.Reset();
	m_rasterizerStateDepthEnabled.Reset();

	m_pass1ScreenSpaceQuad = Renderable();
	m_pass2ScreenSpaceQuad = Renderable();
	
	m_depthTexture.reset();
	m_pass1RenderTargets.clear();
}

void EntityRenderManager::createDeviceDependentResources()
{
	m_rasterizerStateDepthDisabled.Reset();
	m_rasterizerStateDepthEnabled.Reset();

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthClipEnable = false;
	ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_rasterizerStateDepthDisabled.ReleaseAndGetAddressOf()),
		"Create rasterizerStateDepthDisabled");

	// Render using a right handed coordinate system
	rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.FrontCounterClockwise = true;
	ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_rasterizerStateDepthEnabled.ReleaseAndGetAddressOf()),
		"Create rasterizerStateDepthEnabled");

	// Depth buffer settings
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, m_depthStencilStateDepthDisabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthDisabled");

	depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, m_depthStencilStateDepthEnabled.ReleaseAndGetAddressOf()),
		"Create depthStencilStateDepthEnabled");

	// Initial settings
	setDepthEnabled(false);

	// Deffered Lighting Material
	m_deferredLightMaterial = std::make_shared<DeferredLightMaterial>();

	// Full-screen quads
	m_pass1ScreenSpaceQuad = initScreenSpaceQuad(std::make_shared<ClearGBufferMaterial>());
	m_pass2ScreenSpaceQuad = initScreenSpaceQuad(m_deferredLightMaterial);
}

void EntityRenderManager::createWindowSizeDependentResources()
{
	assert(m_pass1RenderTargets.size() == 0);
	const auto d3dDevice = m_deviceResources.GetD3DDevice();

	// Determine the render target size in pixels.
	const auto &outputSize = m_deviceResources.GetOutputSize();
	const UINT width = std::max<UINT>(outputSize.right - outputSize.left, 1);
	const UINT height = std::max<UINT>(outputSize.bottom - outputSize.top, 1);
	
	for (int i = 0; i < 2; ++i)
	{
		auto target = std::make_unique<TextureRenderTarget>(width, height);
		target->load(d3dDevice);
		m_pass1RenderTargets.push_back(move(target));
	}

	// Create a depth texture resource
	m_depthTexture = std::make_unique<DepthTexture>(m_deviceResources);

	// Assign textures to the deferred light material
	m_deferredLightMaterial->setColor0Texture(m_pass1RenderTargets.at(0));
	m_deferredLightMaterial->setColor1Texture(m_pass1RenderTargets.at(1));
	m_deferredLightMaterial->setDepthTexture(m_depthTexture);

	// Set the viewport.
	auto viewport = m_deviceResources.GetScreenViewport();
	m_deviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);
}

void EntityRenderManager::destroyDeviceDependentResources()
{
	m_rasterizerStateDepthDisabled.Reset();
	m_rasterizerStateDepthEnabled.Reset();
	
	m_pass1ScreenSpaceQuad.rendererData.reset();
	m_pass2ScreenSpaceQuad.rendererData.reset();

	m_depthTexture.reset();
	m_pass1RenderTargets.clear();

	m_deferredLightMaterial.reset();	
	m_fatalError = false;
}

void EntityRenderManager::render()
{
	if (!m_fatalError)
	{
		m_deviceResources.PIXBeginEvent(L"setupPass1");
		setupRenderEntities();
		m_deviceResources.PIXEndEvent();

		m_deviceResources.PIXBeginEvent(L"renderEntities");
		renderEntities();
		m_deviceResources.PIXEndEvent();
	}

	m_deviceResources.PIXBeginEvent(L"setupPass2");
	setupRenderLights();
	m_deviceResources.PIXEndEvent();

	if (!m_fatalError)
	{
		m_deviceResources.PIXBeginEvent(L"renderLights");
		renderLights();
		m_deviceResources.PIXEndEvent();
	}
}

void EntityRenderManager::renderEntities()
{
	// Arrays defined outside of the loop so that their memory is re-used.
	BufferArraySet bufferArraySet;

	std::vector<VertexAttribute> vertexAttributes;

	// Iterate over all entities which have a RenderableComponent
	for (auto iter = m_renderableEntities->begin(); iter != m_renderableEntities->end(); ++iter)
	{
		const auto entity = *iter;
		auto renderable = entity->GetFirstComponentOfType<RenderableComponent>();
		if (!renderable->GetVisible())
			continue;

		try {
			auto material = renderable->GetMaterial().get();
			assert(material != nullptr);

			const RendererData *rendererData = renderable->GetRendererData().get();
			if (rendererData == nullptr) {
				const auto meshDataComponent = entity->GetFirstComponentOfType<MeshDataComponent>();
				if (meshDataComponent == nullptr || meshDataComponent->getMeshData() == nullptr)
				{
					// There is no mesh data, we can't render.
					continue;
				}
				const auto &meshData = meshDataComponent->getMeshData();

				vertexAttributes.clear();
				material->getVertexAttributes(vertexAttributes);

				std::unique_ptr<RendererData> rendererDataPtr = createRendererData(*meshData, vertexAttributes);
				rendererData = rendererDataPtr.get();
				renderable->SetRendererData(rendererDataPtr);
			}

			drawRendererData(m_cameraData, entity->WorldTransform(), rendererData, material, bufferArraySet);
		} 
		catch (std::runtime_error &e)
		{
			renderable->SetVisible(false);
			LOG(WARNING) << "Failed to render mesh on entity with ID " << entity->GetId() << ".\n" << e.what();
		}
	}
}

void EntityRenderManager::renderLights()
{
	const auto identity = XMMatrixIdentity();
	BufferArraySet bufferArraySet;

	try
	{
		const auto rendererData = m_pass2ScreenSpaceQuad.rendererData.get();
		if (!rendererData || !rendererData->m_vertexCount)
			return;

		loadRendererDataToDeviceContext(rendererData, bufferArraySet);
		for (auto eIter = m_lightEntities->begin(); eIter != m_lightEntities->end(); ++eIter)
		{
			const auto lightEntity = *eIter;
			bool foundLight = false;
			
			const auto directionalLight = lightEntity->GetFirstComponentOfType<DirectionalLightComponent>();
			if (directionalLight)
			{
				const auto lightDirection = Vector3::Transform(Vector3::Forward, lightEntity->WorldRotation());
				m_deferredLightMaterial->SetupDirectionalLight(lightDirection, directionalLight->getColor(), directionalLight->getIntensity());
				foundLight = true;
			}
			
			const auto pointLight = lightEntity->GetFirstComponentOfType<PointLightComponent>();
			if (pointLight)
			{
				m_deferredLightMaterial->SetupPointLight(lightEntity->WorldPosition(), pointLight->getColor(), pointLight->getIntensity());
				foundLight = true;
			}

			if (foundLight)
			{
				if (m_deferredLightMaterial->render(*rendererData, identity, m_cameraData.viewMatrix, m_cameraData.projectionMatrix, m_deviceResources))
					m_deviceResources.GetD3DDeviceContext()->DrawIndexed(rendererData->m_indexCount, 0, 0);
			}
		}
		m_deferredLightMaterial->unbind(m_deviceResources);
	}
	catch (std::runtime_error &e)
	{
		m_fatalError = true;
		LOG(WARNING) << "Failed to render lights.\n" << e.what();
	}
}

void EntityRenderManager::setDepthEnabled(bool enabled)
{
	const auto deviceContext = m_deviceResources.GetD3DDeviceContext();
	if (enabled)
	{
		deviceContext->RSSetState(m_rasterizerStateDepthEnabled.Get());
		deviceContext->OMSetDepthStencilState(m_depthStencilStateDepthEnabled.Get(), 0);
	}
	else
	{
		deviceContext->RSSetState(m_rasterizerStateDepthDisabled.Get());
		deviceContext->OMSetDepthStencilState(m_depthStencilStateDepthDisabled.Get(), 0);
	}
}

void EntityRenderManager::loadRendererDataToDeviceContext(const RendererData *rendererData, BufferArraySet &bufferArraySet) const
{
	const auto deviceContext = m_deviceResources.GetD3DDeviceContext();

	// Send the buffers to the input assembler
	if (!rendererData->m_vertexBuffers.empty()) {
		auto &vertexBuffers = rendererData->m_vertexBuffers;
		const auto numBuffers = vertexBuffers.size();
		if (numBuffers > 1) {
			bufferArraySet.bufferArray.clear();
			bufferArraySet.strideArray.clear();
			bufferArraySet.offsetArray.clear();

			for (std::vector<MeshBufferAccessor>::size_type i = 0; i < numBuffers; ++i)
			{
				const D3DBufferAccessor *vertexBufferDesc = rendererData->m_vertexBuffers[i].get();
				bufferArraySet.bufferArray.push_back(vertexBufferDesc->m_buffer->m_d3dBuffer);
				bufferArraySet.strideArray.push_back(vertexBufferDesc->m_stride);
				bufferArraySet.offsetArray.push_back(vertexBufferDesc->m_offset);
			}

			deviceContext->IASetVertexBuffers(0, static_cast<UINT>(numBuffers), bufferArraySet.bufferArray.data(), bufferArraySet.strideArray.data(), bufferArraySet.offsetArray.data());
		}
		else
		{
			const D3DBufferAccessor *vertexBufferDesc = rendererData->m_vertexBuffers[0].get();
			deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDesc->m_buffer->m_d3dBuffer, &vertexBufferDesc->m_stride, &vertexBufferDesc->m_offset);
		}

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		deviceContext->IASetPrimitiveTopology(rendererData->m_topology);

		if (rendererData->m_indexBufferAccessor != nullptr)
		{
			// Set the index buffer to active in the input assembler so it can be rendered.
			const auto indexAccessor = rendererData->m_indexBufferAccessor.get();
			deviceContext->IASetIndexBuffer(indexAccessor->m_buffer->m_d3dBuffer, rendererData->m_indexFormat, indexAccessor->m_offset);
		}
	}
}

void EntityRenderManager::drawRendererData(
	const CameraData &cameraData,
	const Matrix &worldTransform,
	const RendererData *rendererData, 
	Material* material,
	BufferArraySet &bufferArraySet) const
{
	if (rendererData->m_vertexBuffers.empty())
		return;

	const auto deviceContext = m_deviceResources.GetD3DDeviceContext();
	loadRendererDataToDeviceContext(rendererData, bufferArraySet);

	if (rendererData->m_indexBufferAccessor != nullptr)
	{
		if (material->render(*rendererData, worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, m_deviceResources))
			deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
	}
	else
	{
		material->render(*rendererData, worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, m_deviceResources);

		// Render the triangles.
		deviceContext->Draw(rendererData->m_vertexCount, rendererData->m_vertexCount);
	}
}


std::unique_ptr<RendererData> EntityRenderManager::createRendererData(const MeshData &meshData, const std::vector<VertexAttribute> &vertexAttributes) const
{
	auto rendererData = std::make_unique<RendererData>();
	rendererData->m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Create D3D Index Buffer
	if (meshData.m_indexBufferAccessor) {
		rendererData->m_indexCount = meshData.m_indexBufferAccessor->m_count;

		rendererData->m_indexBufferAccessor = std::make_unique<D3DBufferAccessor>(
			createBufferFromData(*meshData.m_indexBufferAccessor->m_buffer, D3D11_BIND_INDEX_BUFFER),
			meshData.m_indexBufferAccessor->m_stride,
			meshData.m_indexBufferAccessor->m_offset);
		rendererData->m_indexFormat = meshData.m_indexBufferAccessor->m_format;

		std::string name("Index Buffer (count: " + std::to_string(rendererData->m_indexCount) + ")");
		rendererData->m_indexBufferAccessor->m_buffer->m_d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else 
	{
		// TODO: Simply log a warning?
		rendererData->m_indexCount = 0;
		throw std::runtime_error("CreateRendererData: Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->m_vertexCount = meshData.getVertexCount();
	for (auto vertexAttr : vertexAttributes)
	{
		const auto vabaPos = meshData.m_vertexBufferAccessors.find(vertexAttr);
		if (vabaPos == meshData.m_vertexBufferAccessors.end())
			throw std::runtime_error("CreateRendererData: Missing vertex attribute: "s + VertexAttributeMeta::semanticName(vertexAttr));

		const auto &meshAccessor = vabaPos->second;
		auto d3dAccessor = std::make_unique<D3DBufferAccessor>(
			createBufferFromData(*meshAccessor->m_buffer, D3D11_BIND_VERTEX_BUFFER),
			meshAccessor->m_stride, 
			meshAccessor->m_offset);

		rendererData->m_vertexBuffers.push_back(std::move(d3dAccessor));
	}

	return rendererData;
}

EntityRenderManager::Renderable EntityRenderManager::initScreenSpaceQuad(std::shared_ptr<Material> material) const
{
	Renderable renderable;
	if (renderable.meshData == nullptr)
		renderable.meshData = m_primitiveMeshDataFactory->createQuad(Vector2(2.0f, 2.0f), Vector3(-1.f, -1.f, 0.f));

	if (renderable.material == nullptr)
		renderable.material = material;

	if (renderable.rendererData == nullptr) 
	{
		std::vector<VertexAttribute> vertexAttributes;
		renderable.material->getVertexAttributes(vertexAttributes);
		renderable.rendererData = createRendererData(*renderable.meshData, vertexAttributes);
	}

	return renderable;
}

void EntityRenderManager::setupRenderEntities()
{
	// Clear the views.
	auto context = m_deviceResources.GetD3DDeviceContext();

	const auto depthStencil = m_deviceResources.GetDepthStencilView();
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	assert(m_pass1RenderTargets.size() == 2);
	ID3D11RenderTargetView *renderTargets[2] = {
		m_pass1RenderTargets[0]->getRenderTargetView(),
		m_pass1RenderTargets[1]->getRenderTargetView(),
	};

	context->OMSetRenderTargets(2, renderTargets, depthStencil);
	
	// Clear the rendered textures (ignoring depth)
	if (m_pass1ScreenSpaceQuad.rendererData && m_pass1ScreenSpaceQuad.material) {
		const auto identity = XMMatrixIdentity();
		BufferArraySet bufferArraySet;

		try
		{
			drawRendererData(CameraData::Identity, identity, m_pass1ScreenSpaceQuad.rendererData.get(), m_pass1ScreenSpaceQuad.material.get(), bufferArraySet);
		}
		catch (std::runtime_error &e)
		{
			m_fatalError = true;
			LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
		}
	}

	setDepthEnabled(true);
}

void EntityRenderManager::setupRenderLights()
{
	// Clear the views.
	auto context = m_deviceResources.GetD3DDeviceContext();
	
	context->ClearRenderTargetView(m_deviceResources.GetRenderTargetView(), Colors::Black);

	const auto finalColorRenderTarget = m_deviceResources.GetRenderTargetView();
	
	setDepthEnabled(false);
	context->OMSetRenderTargets(1, &finalColorRenderTarget, nullptr);
}

std::shared_ptr<D3DBuffer> EntityRenderManager::createBufferFromData(const MeshBuffer &buffer, UINT bindFlags) const
{
	// Create the vertex buffer.
	auto d3dBuffer = std::make_shared<D3DBuffer>();

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = static_cast<UINT>(buffer.m_dataSize);
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = buffer.m_data;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Get a reference
	HRESULT hr = m_deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &d3dBuffer->m_d3dBuffer);
	assert(!FAILED(hr));

	return d3dBuffer;
}

std::unique_ptr<Material> EntityRenderManager::loadMaterial(const std::string &materialName) const
{
	return m_materialRepository->instantiate(materialName);
}
