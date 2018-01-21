#include "pch.h"
#include "EntityRenderManager.h"
#include "SceneGraphManager.h"
#include "RendererData.h"
#include "Material.h"
#include "Entity.h"
#include "EntityFilter.h"
#include "Component.h"
#include "Scene.h"

#include <set>
#include <functional>
#include "LightComponent.h"
#include "ClearGBufferMaterial.h"
#include "TextureRenderTarget.h"
#include "RenderableComponent.h"
#include "DeferredLightMaterial.h"
#include <minwinbase.h>

using namespace OE;
using namespace DirectX;
using namespace std::literals;

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
	, m_pass1RasterizerState(nullptr)
	, m_pass2RasterizerState(nullptr)
{
}

EntityRenderManager::~EntityRenderManager()
{
}

void EntityRenderManager::Initialize()
{
	std::set<Component::ComponentType> requiredTypes;
	requiredTypes.insert(RenderableComponent::Type());
	m_renderableEntities = m_scene.GetSceneGraphManager().GetEntityFilter(requiredTypes);

	requiredTypes.clear();
	requiredTypes.insert(DirectionalLightComponent::Type());
	m_lightEntities = m_scene.GetSceneGraphManager().GetEntityFilter(requiredTypes);

	m_primitiveMeshDataFactory = std::make_unique<PrimitiveMeshDataFactory>();
}

void EntityRenderManager::Tick() {
	// Update camera etc.
}

void EntityRenderManager::Shutdown()
{
	if (m_primitiveMeshDataFactory)
		m_primitiveMeshDataFactory.reset();

	m_pass1RasterizerState.Reset();
	m_pass2RasterizerState.Reset();

	m_pass1ScreenSpaceQuad = Renderable();
	m_pass2ScreenSpaceQuad = Renderable();
}

void EntityRenderManager::createDeviceDependentResources()
{
	m_pass1RasterizerState.Reset();
	m_pass2RasterizerState.Reset();

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.DepthClipEnable = false;
	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_pass1RasterizerState.ReleaseAndGetAddressOf()));

	// Render using a right handed coordinate system
	rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.FrontCounterClockwise = true;
	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_pass2RasterizerState.ReleaseAndGetAddressOf()));

	// Depth buffer settings
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, m_pass1DepthStencilState.ReleaseAndGetAddressOf()));

	depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateDepthStencilState(&depthStencilDesc, m_pass2DepthStencilState.ReleaseAndGetAddressOf()));

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

	m_deferredLightMaterial->setColor0Texture(m_pass1RenderTargets.at(0));
	m_deferredLightMaterial->setColor1Texture(m_pass1RenderTargets.at(1));
}

void EntityRenderManager::destroyDeviceDependentResources()
{
	m_pass1RasterizerState.Reset();
	m_pass2RasterizerState.Reset();

	m_pass1RenderTargets.clear();

	m_pass1ScreenSpaceQuad.rendererData.reset();
	m_pass2ScreenSpaceQuad.rendererData.reset();

	m_deferredLightMaterial.reset();	
	m_fatalError = false;
}

void EntityRenderManager::render()
{
	// Set the viewport.
	auto viewport = m_deviceResources.GetScreenViewport();
	m_deviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);

	if (!m_fatalError)
	{
		m_deviceResources.PIXBeginEvent(L"setupPass1");
		setupPass1();
		m_deviceResources.PIXEndEvent();

		m_deviceResources.PIXBeginEvent(L"renderEntities");
		renderEntities();
		m_deviceResources.PIXEndEvent();
	}

	m_deviceResources.PIXBeginEvent(L"setupPass2");
	setupPass2();
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
	auto deviceContext = m_deviceResources.GetD3DDeviceContext();
	
	// Hard Coded Camera
	const DirectX::XMMATRIX &viewMatrix = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(5.0f, 3.0f, -10.0f, 0.0f), Math::VEC_ZERO, Math::VEC_UP);
	const auto viewport = m_deviceResources.GetScreenViewport();
	const float aspectRatio = viewport.Width / viewport.Height;
	const auto projMatrix = DirectX::XMMatrixPerspectiveFovRH(
		DirectX::XMConvertToRadians(45.0f),
		aspectRatio,
		0.01f,
		10000.0f);

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

			drawRendererData(viewMatrix, projMatrix, entity->GetWorldTransform(), rendererData, material, bufferArraySet);
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
		drawRendererData(identity, identity, identity, m_pass2ScreenSpaceQuad.rendererData.get(), m_pass2ScreenSpaceQuad.material.get(), bufferArraySet);
		m_deferredLightMaterial->unbind(m_deviceResources);
	}
	catch (std::runtime_error &e)
	{
		m_fatalError = true;
		LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
	}
}

void EntityRenderManager::drawRendererData(const DirectX::XMMATRIX &viewMatrix, const DirectX::XMMATRIX &projMatrix,
	const DirectX::XMMATRIX &worldTransform,
	const RendererData *rendererData, Material* material,
	BufferArraySet &bufferArraySet)
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

			if (material->render(*rendererData, worldTransform, viewMatrix, projMatrix, m_deviceResources))
				deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
		}
		else
		{
			material->render(*rendererData, worldTransform, viewMatrix, projMatrix, m_deviceResources);

			// Render the triangles.
			deviceContext->Draw(rendererData->m_vertexCount, rendererData->m_vertexCount);
		}
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
		renderable.meshData = m_primitiveMeshDataFactory->createQuad(Rect(-1.f, -1.f, 2.0f, 2.0f));

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

void EntityRenderManager::setupPass1()
{
	// Clear the views.
	auto context = m_deviceResources.GetD3DDeviceContext();

	const auto depthStencil = nullptr;// m_deviceResources.GetDepthStencilView();

	assert(m_pass1RenderTargets.size() == 2);
	ID3D11RenderTargetView *renderTargets[2] = {
		m_pass1RenderTargets[0]->getRenderTargetView(),
		m_pass1RenderTargets[1]->getRenderTargetView(),
	};

	context->RSSetState(m_pass1RasterizerState.Get());
	context->OMSetDepthStencilState(m_pass1DepthStencilState.Get(), 0);
	context->OMSetRenderTargets(2, renderTargets, depthStencil);
	
	// Clear the rendered textures (ignoring depth)
	if (m_pass1ScreenSpaceQuad.rendererData && m_pass1ScreenSpaceQuad.material) {
		const auto identity = XMMatrixIdentity();
		BufferArraySet bufferArraySet;

		try
		{
			drawRendererData(identity, identity, identity, m_pass1ScreenSpaceQuad.rendererData.get(), m_pass1ScreenSpaceQuad.material.get(), bufferArraySet);
		}
		catch (std::runtime_error &e)
		{
			m_fatalError = true;
			LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
		}
	}

	// Clear the depth buffer
	//context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void EntityRenderManager::setupPass2()
{
	// Clear the views.
	auto context = m_deviceResources.GetD3DDeviceContext();

	const auto depthStencil = m_deviceResources.GetDepthStencilView();
	const auto finalColorRenderTarget = m_deviceResources.GetRenderTargetView();

	context->ClearRenderTargetView(finalColorRenderTarget, Colors::BlanchedAlmond);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	context->OMSetDepthStencilState(m_pass2DepthStencilState.Get(), 0);
	context->RSSetState(m_pass2RasterizerState.Get());
	context->OMSetRenderTargets(1, &finalColorRenderTarget, depthStencil);
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
