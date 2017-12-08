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

using namespace OE;
using namespace DirectX;

EntityRenderManager::EntityRenderManager(Scene &scene)
	: ManagerBase(scene)
{
}

EntityRenderManager::~EntityRenderManager()
{
}

void EntityRenderManager::Initialize() 
{
	std::set<Component::ComponentType> filter;
	filter.insert(RenderableComponent::Type());
	entityFilter = m_scene.GetEntityManager().GetEntityFilter(filter);
}

void EntityRenderManager::Tick() {
	// Update camera etc.
}

void EntityRenderManager::CreateDeviceDependentResources(const DX::DeviceResources &deviceResources)
{
}

void EntityRenderManager::CreateWindowSizeDependentResources(const DX::DeviceResources &deviceResources)
{
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
		1000.0f);

	auto deviceContext = deviceResources.GetD3DDeviceContext();

	for (auto iter = entityFilter->begin(); iter != entityFilter->end(); ++iter)
	{
		auto entity = *iter;
		auto renderable = entity->GetFirstComponentOfType<RenderableComponent>();
		if (!renderable->GetVisible())
			continue;

		try {
			// TODO: Don't look up the mesh data by name in a map each frame... perhaps cache it on the component? Are we allowing that?
			const std::string &meshName = renderable->GetMeshName();
			auto meshRendererDataPos = m_meshRendererData.find(meshName);
			if (meshRendererDataPos == m_meshRendererData.end()) {
				m_meshRendererData[meshName] = std::move(CreateRendererData(deviceResources));
			}
			RendererData* rendererData = m_meshRendererData[meshName].get();

			// Send the buffers to the input assembler
			if (rendererData->m_vertexBuffers.size()) {
				auto &vertexBuffers = rendererData->m_vertexBuffers;
				for (std::vector<VertexBufferAccessor>::size_type i = 0; i < vertexBuffers.size(); ++i)
				{
					const VertexBufferAccessor &vertexBufferDesc = *rendererData->m_vertexBuffers[i].get();
					
					deviceContext->IASetVertexBuffers(static_cast<UINT>(i), 1, &vertexBufferDesc.m_buffer, &vertexBufferDesc.m_stride, &vertexBufferDesc.m_offset);
				}

				// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
				deviceContext->IASetPrimitiveTopology(rendererData->m_topology);

				const std::string &materialName = renderable->GetMaterialName();
				auto materialPos = m_materials.find(materialName);
				if (materialPos == m_materials.end()) {
					m_materials[materialName] = std::move(LoadMaterial(materialName));
				}
				Material *material = m_materials[materialName].get();
				assert(material != nullptr);

				if (rendererData->m_indexBuffer)
				{
					// Set the index buffer to active in the input assembler so it can be rendered.
					deviceContext->IASetIndexBuffer(rendererData->m_indexBuffer, DXGI_FORMAT_R32_UINT, rendererData->m_indexBufferOffset);

					if (material->Render(entity->GetWorldTransform(), viewMatrix, projMatrix, deviceResources))
						deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
				}
				else
				{
					material->Render(entity->GetWorldTransform(), viewMatrix, projMatrix, deviceResources);

					// Render the triangles.
					deviceContext->Draw(rendererData->m_vertexCount, rendererData->m_vertexCount);
				}
			}
		} 
		catch (...)
		{
			renderable->SetVisible(false);
		}
	}
}


struct SimpleVertexCombined
{
	XMFLOAT3 Pos;
	XMFLOAT3 Col;
};

std::unique_ptr<RendererData> EntityRenderManager::CreateRendererData(const DX::DeviceResources deviceResources)
{
	auto rendererData = std::make_unique<RendererData>();
	{
		// Supply the actual vertex data.
		constexpr float size = 1.0f;
		constexpr unsigned int NUM_VERTICES = 8;
		SimpleVertexCombined verticesCombo[] =
		{
			XMFLOAT3(size,  size, -size), XMFLOAT3(1.0f, 1.0f, 1.0f),
			XMFLOAT3(-size,  size, -size), XMFLOAT3(0.0f, 1.0f, 1.0f),
			XMFLOAT3(size, -size, -size), XMFLOAT3(1.0f, 0.0f, 1.0f),
			XMFLOAT3(-size, -size, -size), XMFLOAT3(0.0f, 0.0f, 1.0f),

			XMFLOAT3(size,  size,  size), XMFLOAT3(1.0f, 1.0f, 0.0f),
			XMFLOAT3(-size,  size,  size), XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3(size, -size,  size), XMFLOAT3(1.0f, 0.0f, 0.0f),
			XMFLOAT3(-size, -size,  size), XMFLOAT3(0.0f, 0.0f, 0.0f),
		};

		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(SimpleVertexCombined) * NUM_VERTICES;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = verticesCombo;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the vertex buffer.
		rendererData->m_vertexBuffers.push_back(std::make_unique<VertexBufferAccessor>());
		VertexBufferAccessor &vertexBufferDesc = *rendererData->m_vertexBuffers.rbegin()->get();

		// Get a reference
		vertexBufferDesc.m_stride = sizeof(SimpleVertexCombined);
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &vertexBufferDesc.m_buffer);
		assert(!FAILED(hr));
	}

	// ---------------
	{
		// Create indices.
		//unsigned int indices[] = { 0, 1, 2 };
		const int NUM_INDICES = 36;
		unsigned int indices[NUM_INDICES];

		int pos = 0;
		// -X side
		indices[pos++] = 5; indices[pos++] = 7; indices[pos++] = 3;
		indices[pos++] = 5; indices[pos++] = 3; indices[pos++] = 1;

		// +X side
		indices[pos++] = 6; indices[pos++] = 4; indices[pos++] = 0;
		indices[pos++] = 6; indices[pos++] = 0; indices[pos++] = 2;

		// -Y side
		indices[pos++] = 7; indices[pos++] = 6; indices[pos++] = 2;
		indices[pos++] = 7; indices[pos++] = 2; indices[pos++] = 3;

		// +Y side
		indices[pos++] = 4; indices[pos++] = 5; indices[pos++] = 1;
		indices[pos++] = 4; indices[pos++] = 1; indices[pos++] = 0;

		// -Z side
		indices[pos++] = 5; indices[pos++] = 4; indices[pos++] = 6;
		indices[pos++] = 5; indices[pos++] = 6; indices[pos++] = 7;

		// +Z side
		indices[pos++] = 3; indices[pos++] = 2; indices[pos++] = 0;
		indices[pos++] = 3; indices[pos++] = 0; indices[pos++] = 1;


		// Fill in a buffer description.
		rendererData->m_indexCount = NUM_INDICES;
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(unsigned int) * NUM_INDICES;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Define the resource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = indices;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer with the device.
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &rendererData->m_indexBuffer);
		assert(!FAILED(hr));
	}

	return rendererData;
}

std::unique_ptr<Material> EntityRenderManager::LoadMaterial(const std::string &materialName) {
	// TODO: Look up by name. For now, hard coded!!
	materialName;

	return std::make_unique<Material>();
}