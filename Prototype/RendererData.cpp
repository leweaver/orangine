#include "pch.h"
#include "RendererData.h"

using namespace OE;

VertexBufferAccessor::VertexBufferAccessor()
	: m_buffer(nullptr)
	, m_stride(0)
	, m_offset(0)
{
}

VertexBufferAccessor::~VertexBufferAccessor()
{
	if (m_buffer)
		m_buffer->Release();
	m_buffer = nullptr;
}

RendererData::RendererData()
	: m_vertexCount(0)
	, m_indexBuffer(nullptr)
	, m_indexCount(0)
	, m_indexBufferOffset(0)
	, m_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

RendererData::~RendererData()
{
	Release();
}

void RendererData::Release()
{
	m_vertexBuffers.clear();

	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}
}
