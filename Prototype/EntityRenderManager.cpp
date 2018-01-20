#include "pch.h"
#include "EntityRenderManager.h"
#include "SceneGraphManager.h"
#include "RendererData.h"
#include "Material.h"
#include "Entity.h"
#include "EntityFilter.h"
#include "RenderableComponent.h"
#include "Scene.h"

#include <set>
#include <functional>
#include "LightComponent.h"
#include "ClearGBufferMaterial.h"

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
	, m_rasterizerState(nullptr)
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

	if (m_rasterizerState)
		m_rasterizerState->Release();

	m_screenSpaceQuad = Renderable();
}

void EntityRenderManager::createDeviceDependentResources()
{
	if (m_rasterizerState)
		m_rasterizerState->Release();

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	// Render using a right handed coordinate system
	rasterizerDesc.FrontCounterClockwise = true;

	m_deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
	m_deviceResources.GetD3DDeviceContext()->RSSetState(m_rasterizerState);

	initScreenSpaceQuad();
}

void EntityRenderManager::createWindowSizeDependentResources()
{
}

void EntityRenderManager::destroyDeviceDependentResources()
{
	if (m_rasterizerState)
		m_rasterizerState->Release();
	m_rasterizerState = nullptr;

	m_screenSpaceQuad.rendererData.reset();
}

void EntityRenderManager::render()
{
	// Hard Coded Camera
	const DirectX::XMMATRIX &viewMatrix = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(5.0f, 3.0f, -10.0f, 0.0f), Math::VEC_ZERO, Math::VEC_UP);
	const float aspectRatio = m_deviceResources.GetScreenViewport().Width / m_deviceResources.GetScreenViewport().Height;
	const auto projMatrix = DirectX::XMMatrixPerspectiveFovRH(
		DirectX::XMConvertToRadians(45.0f),
		aspectRatio,
		0.01f,
		10000.0f);

	auto deviceContext = m_deviceResources.GetD3DDeviceContext();

	std::vector<ID3D11Buffer*> bufferArray;
	std::vector<UINT> strideArray;
	std::vector<UINT> offsetArray;

	std::vector<VertexAttribute> vertexAttributes;

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

				std::unique_ptr<RendererData> rendererDataPtr = CreateRendererData(*meshData, vertexAttributes);
				rendererData = rendererDataPtr.get();
				renderable->SetRendererData(rendererDataPtr);
			}

			// Send the buffers to the input assembler
			if (!rendererData->m_vertexBuffers.empty()) {
				auto &vertexBuffers = rendererData->m_vertexBuffers;
				const auto numBuffers = vertexBuffers.size();
				if (numBuffers > 1) {
					bufferArray.clear();
					strideArray.clear();
					offsetArray.clear();

					for (std::vector<MeshBufferAccessor>::size_type i = 0; i < numBuffers; ++i)
					{
						const D3DBufferAccessor *vertexBufferDesc = rendererData->m_vertexBuffers[i].get();
						bufferArray.push_back(vertexBufferDesc->m_buffer->m_d3dBuffer);
						strideArray.push_back(vertexBufferDesc->m_stride);
						offsetArray.push_back(vertexBufferDesc->m_offset);
					}

					deviceContext->IASetVertexBuffers(0, static_cast<UINT>(numBuffers), bufferArray.data(), strideArray.data(), offsetArray.data());
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

					if (material->render(*rendererData, entity->GetWorldTransform(), viewMatrix, projMatrix, m_deviceResources))
						deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
				}
				else
				{
					material->render(*rendererData, entity->GetWorldTransform(), viewMatrix, projMatrix, m_deviceResources);

					// Render the triangles.
					deviceContext->Draw(rendererData->m_vertexCount, rendererData->m_vertexCount);
				}
			}
		} 
		catch (std::runtime_error &e)
		{
			renderable->SetVisible(false);
			LOG(WARNING) << "Failed to render mesh on entity with ID " << entity->GetId() << ".\n" << e.what();
		}
	}
}

std::unique_ptr<RendererData> EntityRenderManager::CreateRendererData(const MeshData &meshData, const std::vector<VertexAttribute> &vertexAttributes) const
{
	auto rendererData = std::make_unique<RendererData>();
	rendererData->m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Create D3D Index Buffer
	if (rendererData->m_indexBufferAccessor) {
		rendererData->m_indexCount = meshData.m_indexBufferAccessor->m_count;

		rendererData->m_indexBufferAccessor = std::make_unique<D3DBufferAccessor>(
			CreateBufferFromData(*meshData.m_indexBufferAccessor->m_buffer, D3D11_BIND_INDEX_BUFFER),
			meshData.m_indexBufferAccessor->m_stride,
			meshData.m_indexBufferAccessor->m_offset);
		rendererData->m_indexFormat = meshData.m_indexBufferAccessor->m_format;

		std::string name("Index Buffer (count: " + std::to_string(rendererData->m_indexCount) + ")");
		rendererData->m_indexBufferAccessor->m_buffer->m_d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else {
		// TODO: Simply log a warning?
		rendererData->m_indexCount = 0;
		throw std::runtime_error("Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->m_vertexCount = meshData.getVertexCount();
	for (auto vertexAttr : vertexAttributes)
	{
		const auto vabaPos = meshData.m_vertexBufferAccessors.find(vertexAttr);
		if (vabaPos == meshData.m_vertexBufferAccessors.end())
			throw std::runtime_error("Missing vertex attribute: "s + VertexAttributeMeta::semanticName(vertexAttr));

		const auto &meshAccessor = vabaPos->second;
		auto d3dAccessor = std::make_unique<D3DBufferAccessor>(
			CreateBufferFromData(*meshAccessor->m_buffer, D3D11_BIND_VERTEX_BUFFER),
			meshAccessor->m_stride, 
			meshAccessor->m_offset);

		rendererData->m_vertexBuffers.push_back(std::move(d3dAccessor));
	}

	return rendererData;
}

void EntityRenderManager::initScreenSpaceQuad()
{
	if (m_screenSpaceQuad.meshData == nullptr)
		m_screenSpaceQuad.meshData = m_primitiveMeshDataFactory->createQuad(Rect(0.f, 0.f, 1.0f, 1.0f));

	if (m_screenSpaceQuad.material == nullptr)
		m_screenSpaceQuad.material = std::make_shared<ClearGBufferMaterial>();

	if (m_screenSpaceQuad.rendererData == nullptr) {
		std::vector<VertexAttribute> vertexAttributes;
		m_screenSpaceQuad.material->getVertexAttributes(vertexAttributes);
		m_screenSpaceQuad.rendererData = CreateRendererData(*m_screenSpaceQuad.meshData, vertexAttributes);
	}
}

void EntityRenderManager::clearGBuffer()
{
	m_deviceResources.PIXBeginEvent(L"Clear");

	// Clear the views.
	auto context = m_deviceResources.GetD3DDeviceContext();
	auto renderTarget = m_deviceResources.GetRenderTargetView();
	auto depthStencil = m_deviceResources.GetDepthStencilView();

	context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

	// Set the viewport.
	auto viewport = m_deviceResources.GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources.PIXEndEvent();

	if (m_screenSpaceQuad.rendererData == nullptr) {
		LOG(WARNING) << "cannot clear G-Buffer; screen space quad rendererData is null.";
		return;
	}	
}

std::shared_ptr<D3DBuffer> EntityRenderManager::CreateBufferFromData(const MeshBuffer &buffer, UINT bindFlags) const
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

std::unique_ptr<Material> EntityRenderManager::LoadMaterial(const std::string &materialName) const
{
	return m_materialRepository->instantiate(materialName);
}
