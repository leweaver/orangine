#include "pch.h"
#include "RendererData.h"

using namespace OE;

D3DBuffer::D3DBuffer(ID3D11Buffer *buffer)
	: m_d3dBuffer(buffer)
{}

D3DBuffer::D3DBuffer()
	: m_d3dBuffer(nullptr)
{}

D3DBuffer::~D3DBuffer()
{
	if (m_d3dBuffer != nullptr)
		m_d3dBuffer->Release();
	m_d3dBuffer = nullptr;
}

D3DBufferAccessor::D3DBufferAccessor(const std::shared_ptr<D3DBuffer> &buffer, unsigned stride, unsigned offset)
	: m_buffer(buffer)
	, m_stride(stride)
	, m_offset(offset)
{
}

D3DBufferAccessor::~D3DBufferAccessor()
{
	m_buffer = nullptr;
}

RendererData::RendererData()
	: m_vertexCount(0)
	, m_indexFormat(DXGI_FORMAT_UNKNOWN)
	, m_indexCount(0)
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
	m_indexBufferAccessor.release();
}
