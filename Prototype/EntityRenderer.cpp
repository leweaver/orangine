#include "pch.h"
#include "EntityRenderer.h"
#include "RendererData.h"

using namespace OE;

EntityRenderer::EntityRenderer(Scene& scene)
	: m_scene(scene)
{
}

EntityRenderer::~EntityRenderer()
{
}

void EntityRenderer::CreateDeviceDependentResources(const DX::DeviceResources& deviceResources)
{
}

void EntityRenderer::CreateWindowSizeDependentResources(const DX::DeviceResources& deviceResources)
{
}

void EntityRenderer::Render(const DX::DeviceResources& deviceResources)
{
	for (RenderableComponent* renderable : m_renderables)
	{
		RendererData* data = renderable->GetRendererData();
		if (data == nullptr)
			renderable->CreateRendererData(deviceResources);
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
