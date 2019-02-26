#include "pch.h"
#include "Renderer_data.h"

using namespace oe;

D3D_buffer::D3D_buffer(ID3D11Buffer *buffer, size_t size)
	: d3dBuffer(buffer)
    , size(size)
{}

D3D_buffer::D3D_buffer()
	: d3dBuffer(nullptr)
    , size(0)
{}

D3D_buffer::~D3D_buffer()
{
	if (d3dBuffer != nullptr)
		d3dBuffer->Release();
}

D3D_buffer_accessor::D3D_buffer_accessor(std::shared_ptr<D3D_buffer> buffer, uint32_t stride, uint32_t offset)
	: buffer(std::move(buffer))
	, stride(stride)
	, offset(offset)
{
}

D3D_buffer_accessor::~D3D_buffer_accessor()
{
	buffer.reset();
}

Renderer_data::Renderer_data()
	: vertexCount(0)
	, indexFormat(DXGI_FORMAT_UNKNOWN)
	, indexCount(0)
	, topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	, failedRendering(false)
{
}

Renderer_data::~Renderer_data()
{
	release();
}

void Renderer_data::release()
{
	vertexBuffers.clear();
	indexBufferAccessor.reset();
}
