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

using namespace OE;
using namespace DirectX;

EntityRenderManager::EntityRenderManager(Scene& scene, const std::shared_ptr<MaterialRepository> &materialRepository)
	: ManagerBase(scene)
	, m_renderableEntities(nullptr)
	, m_materialRepository(materialRepository)
	, m_rasterizerState(nullptr)
{
}

EntityRenderManager::~EntityRenderManager()
{
}

void EntityRenderManager::Initialize() 
{
	std::set<Component::ComponentType> filter;
	filter.insert(RenderableComponent::Type());
	m_renderableEntities = m_scene.GetSceneGraphManager().GetEntityFilter(filter);
}

void EntityRenderManager::Tick() {
	// Update camera etc.
}

void EntityRenderManager::Shutdown()
{
	if (m_rasterizerState)
		m_rasterizerState->Release();
}

void EntityRenderManager::CreateDeviceDependentResources(const DX::DeviceResources &deviceResources)
{
	if (m_rasterizerState)
		m_rasterizerState->Release();

	D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	// Render using a right handed coordinate system
	rasterizerDesc.FrontCounterClockwise = true;

	deviceResources.GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
	deviceResources.GetD3DDeviceContext()->RSSetState(m_rasterizerState);
}

void EntityRenderManager::CreateWindowSizeDependentResources(const DX::DeviceResources &deviceResources)
{
}

void EntityRenderManager::DestroyDeviceDependentResources()
{
	if (m_rasterizerState)
		m_rasterizerState->Release();
	m_rasterizerState = nullptr;
}

void EntityRenderManager::Render(const DX::DeviceResources &deviceResources)
{
	// Hard Coded Camera
	const DirectX::XMMATRIX &viewMatrix = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(5.0f, 3.0f, -10.0f, 0.0f), Math::VEC_ZERO, Math::VEC_UP);
	const float aspectRatio = deviceResources.GetScreenViewport().Width / deviceResources.GetScreenViewport().Height;
	const auto projMatrix = DirectX::XMMatrixPerspectiveFovRH(
		DirectX::XMConvertToRadians(45.0f),
		aspectRatio,
		0.01f,
		10000.0f);

	auto deviceContext = deviceResources.GetD3DDeviceContext();

	std::vector<ID3D11Buffer*> bufferArray;
	std::vector<UINT> strideArray;
	std::vector<UINT> offsetArray;

	for (auto iter = m_renderableEntities->begin(); iter != m_renderableEntities->end(); ++iter)
	{
		const auto entity = *iter;
		auto renderable = entity->GetFirstComponentOfType<RenderableComponent>();
		if (!renderable->GetVisible())
			continue;

		try {
			const RendererData *rendererData = renderable->GetRendererData().get();
			if (rendererData == nullptr) {
				const auto meshData = entity->GetFirstComponentOfType<MeshDataComponent>();
				if (meshData == nullptr)
				{
					// There is no mesh data, we can't render.
					continue;
				}

				std::unique_ptr<RendererData> rendererDataPtr = CreateRendererData(*meshData, deviceResources);
				rendererData = rendererDataPtr.get();
				renderable->SetRendererData(rendererDataPtr);
			}

			// Send the buffers to the input assembler
			if (rendererData->m_vertexBuffers.size()) {
				auto &vertexBuffers = rendererData->m_vertexBuffers;
				const size_t numBuffers = vertexBuffers.size();
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

				Material *material = renderable->GetMaterial().get();
				assert(material != nullptr);

				if (rendererData->m_indexBufferAccessor != nullptr)
				{
					// Set the index buffer to active in the input assembler so it can be rendered.
					const auto indexAccessor = rendererData->m_indexBufferAccessor.get();
					deviceContext->IASetIndexBuffer(indexAccessor->m_buffer->m_d3dBuffer, rendererData->m_indexFormat, indexAccessor->m_offset);

					if (material->render(*rendererData, entity->GetWorldTransform(), viewMatrix, projMatrix, deviceResources))
						deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
				}
				else
				{
					material->render(*rendererData, entity->GetWorldTransform(), viewMatrix, projMatrix, deviceResources);

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

std::unique_ptr<RendererData> EntityRenderManager::CreateRendererData(const MeshDataComponent &meshData, const DX::DeviceResources &deviceResources) const
{
	auto rendererData = std::make_unique<RendererData>();
	rendererData->m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Create D3D Index Buffer
	rendererData->m_indexCount = meshData.m_indexCount;
	if (rendererData->m_indexCount) {
		rendererData->m_indexBufferAccessor = std::make_unique<D3DBufferAccessor>(
			CreateBufferFromData(*meshData.m_indexBufferAccessor->m_buffer, D3D11_BIND_INDEX_BUFFER, deviceResources),
			meshData.m_indexBufferAccessor->m_stride,
			meshData.m_indexBufferAccessor->m_offset);
		rendererData->m_indexFormat = meshData.m_indexBufferAccessor->m_format;

		std::string name("Index Buffer (count: " + std::to_string(meshData.m_indexCount) + ")");
		rendererData->m_indexBufferAccessor->m_buffer->m_d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else {
		// TODO: Simply log a warning?
		throw std::runtime_error("Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->m_vertexCount = meshData.m_vertexCount;
	for (const auto &meshAccessor : meshData.m_vertexBufferAccessors)
	{
		auto d3dAccessor = std::make_unique<D3DBufferAccessor>(
			CreateBufferFromData(*meshAccessor->m_buffer, D3D11_BIND_VERTEX_BUFFER, deviceResources),
			meshAccessor->m_stride, 
			meshAccessor->m_offset);

		rendererData->m_vertexBuffers.push_back(move(d3dAccessor));
	}

	return rendererData;
}

std::shared_ptr<D3DBuffer> EntityRenderManager::CreateBufferFromData(const MeshBuffer &buffer, UINT bindFlags, const DX::DeviceResources &deviceResources) const
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
	HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &d3dBuffer->m_d3dBuffer);
	assert(!FAILED(hr));

	return d3dBuffer;
}

std::unique_ptr<Material> EntityRenderManager::LoadMaterial(const std::string &materialName) const
{
	return m_materialRepository->instantiate(materialName);
}
