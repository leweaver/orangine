#pragma once
#include <vector>
#include <memory>
#include <d3d11.h>

namespace OE 
{
	struct D3DBuffer
	{
		D3DBuffer(ID3D11Buffer *buffer);
		D3DBuffer();
		~D3DBuffer();

		ID3D11Buffer *m_d3dBuffer;
	};

	struct D3DBufferAccessor
	{
		D3DBufferAccessor(const std::shared_ptr<D3DBuffer> &buffer, unsigned stride, unsigned offset);
		~D3DBufferAccessor();

		std::shared_ptr<D3DBuffer> m_buffer;
		unsigned int m_stride;
		unsigned int m_offset;
	};

	class RendererData
	{
	public:
		RendererData();
		~RendererData();

		void Release();

		std::vector<std::unique_ptr<D3DBufferAccessor>> m_vertexBuffers;
		unsigned int m_vertexCount;

		std::unique_ptr<D3DBufferAccessor> m_indexBufferAccessor;
		DXGI_FORMAT m_indexFormat;
		unsigned int m_indexCount;

		D3D11_PRIMITIVE_TOPOLOGY m_topology;
	};
}
