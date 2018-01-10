#include "pch.h"
#include "RenderableComponent.h"

using namespace OE;

DEFINE_COMPONENT_TYPE(RenderableComponent);

RenderableComponent::~RenderableComponent()
{
	m_material.reset();
	m_rendererData.reset();
}