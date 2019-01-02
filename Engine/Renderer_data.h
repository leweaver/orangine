#pragma once
#include <vector>
#include <memory>
#include <d3d11.h>

#include <DirectXCollision.h>

namespace oe 
{
	struct D3D_buffer
	{
		D3D_buffer(ID3D11Buffer* buffer);
		D3D_buffer();
		~D3D_buffer();

		ID3D11Buffer* d3dBuffer;
	};

	struct D3D_buffer_accessor
	{
		D3D_buffer_accessor(std::shared_ptr<D3D_buffer> buffer, uint32_t stride, uint32_t offset);
		~D3D_buffer_accessor();

		std::shared_ptr<D3D_buffer> buffer;
		uint32_t stride;
		uint32_t offset;
	};

	struct Renderer_data
	{
		Renderer_data();
		~Renderer_data();

		void release();

		std::vector<std::unique_ptr<D3D_buffer_accessor>> vertexBuffers;
		unsigned int vertexCount;

		std::unique_ptr<D3D_buffer_accessor> indexBufferAccessor;
		DXGI_FORMAT indexFormat;
		unsigned int indexCount;

		D3D11_PRIMITIVE_TOPOLOGY topology;
		bool failedRendering;
	};
}
