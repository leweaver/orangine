#include "pch.h"
#include "EntityRenderer.h"
#include "RendererData.h"
#include "Material.h"

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
				for (std::vector<VertexBufferDesc>::size_type i = 0; i < vertexBuffers.size(); ++i)
				{
					const VertexBufferDesc &vertexBufferDesc = *rendererData->m_vertexBuffers[i].get();
					deviceContext->IASetVertexBuffers(i, 1, &vertexBufferDesc.m_buffer, &vertexBufferDesc.m_stride, &vertexBufferDesc.m_offset);
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
					
					if (material->Render(deviceResources))
						deviceContext->DrawIndexed(rendererData->m_indexCount, 0, 0);
				}
				else
				{
					auto material = renderable->GetMaterial();
					if (material != nullptr)
						material->Render(deviceResources);

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
