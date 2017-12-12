#pragma once
#include <vector>
#include <memory>
#include <d3d11.h>

namespace OE 
{
	enum VertexAttribute
	{
		VA_POSITION,
		VA_COLOR,
		VA_NORMAL,
		VA_TANGENT,
		VA_TEXCOORD_0,
	};

	struct VertexBuffer
	{
		VertexBuffer(ID3D11Buffer *buffer);
		~VertexBuffer();

		ID3D11Buffer *const m_buffer;
	};

	struct VertexBufferAccessor
	{
		VertexBufferAccessor();
		~VertexBufferAccessor();

		std::shared_ptr<VertexBuffer> m_buffer;
		unsigned int m_stride;
		unsigned int m_offset;
	};

	class RendererData
	{
	public:
		RendererData();
		~RendererData();

		void Release();

		std::vector<std::unique_ptr<VertexBufferAccessor>> m_vertexBuffers;
		unsigned int m_vertexCount;

		ID3D11Buffer* m_indexBuffer;
		unsigned int m_indexCount;
		unsigned int m_indexBufferOffset;
		D3D11_PRIMITIVE_TOPOLOGY m_topology;
	};
}
