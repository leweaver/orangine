#pragma once
#include <vector>

namespace OE {

	struct VertexBufferAccessor
	{
		VertexBufferAccessor();
		~VertexBufferAccessor();

		ID3D11Buffer* m_buffer;
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
