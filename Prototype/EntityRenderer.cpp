#include "pch.h"
#include "EntityRenderer.h"
#include "RendererData.h"
#include "Material.h"
#include "Entity.h"

using namespace OE;

EntityRenderer::EntityRenderer(Scene &scene)
	: m_scene(scene)
{
}

EntityRenderer::~EntityRenderer()
{
}

void EntityRenderer::CreateDeviceDependentResources(const DX::DeviceResources &deviceResources)
{
}

void EntityRenderer::CreateWindowSizeDependentResources(const DX::DeviceResources &deviceResources)
{
}

void EntityRenderer::Render(const DX::DeviceResources &deviceResources)
{
	// Hard Coded Camera
	const DirectX::XMMATRIX &viewMatrix = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(5.0f, 3.0f, -10.0f, 0.0f), Math::VEC_ZERO, Math::VEC_UP);
	const float aspectRatio = deviceResources.GetScreenViewport().Width / deviceResources.GetScreenViewport().Height;
	const auto projMatrix = DirectX::XMMatrixPerspectiveFovRH(
		DirectX::XMConvertToRadians(45.0f),
		aspectRatio,
		0.01f,
		1000.0f);

	for (RenderableComponent* renderable : m_renderables)
	{
		if (!renderable->GetVisible())
			continue;

		try {
			const RendererData *rendererData = renderable->GetRendererData();
			if (rendererData == nullptr)
				rendererData = renderable->CreateRendererData(deviceResources);

			auto deviceContext = deviceResources.GetD3DDeviceContext();

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

				if (rendererData->m_indexBuffer)
				{
					// Set the index buffer to active in the input assembler so it can be rendered.
					deviceContext->IASetIndexBuffer(rendererData->m_indexBuffer, DXGI_FORMAT_R32_UINT, rendererData->m_indexBufferOffset);

					auto material = renderable->GetMaterial();
					if (material == nullptr)
						throw "Missing material for component";
					
					if (material->Render(renderable->GetEntity().GetWorldTransform(), viewMatrix, projMatrix, deviceResources))
						deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
				}
				else
				{
					auto material = renderable->GetMaterial();
					if (material != nullptr)
						material->Render(renderable->GetEntity().GetWorldTransform(), viewMatrix, projMatrix, deviceResources);

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

void EntityRenderer::AddRenderable(RenderableComponent& renderableComponent)
{
	m_renderables.insert(&renderableComponent);
}

void EntityRenderer::RemoveRenderable(RenderableComponent& renderableComponent)
{
	m_renderables.erase(&renderableComponent);
}
