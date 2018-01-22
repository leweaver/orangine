#pragma once

#include "Texture.h"
#include "DirectXTex.h"
#include "DeviceResources.h"

namespace OE {
	class TextureBase : public Texture
	{
		bool m_generateMipMaps;

	protected: 
		TextureBase(uint32_t width, uint32_t height, bool generateMipmaps)
			: Texture(width, height)
			, m_generateMipMaps(generateMipmaps)
		{}

		HRESULT loadShaderResourceView(ID3D11Device *device, const DirectX::TexMetadata &metaData, const DirectX::ScratchImage &scratchImage);
	};

	class BufferTexture : public TextureBase
	{
		std::unique_ptr<uint8_t> m_buffer;

		uint32_t m_stride;
		uint32_t m_bufferSize;
	public:

		BufferTexture(uint32_t width, uint32_t height, uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t> &buffer, bool generateMipmaps = true)
			: TextureBase(width, height, generateMipmaps)
			, m_buffer(std::move(buffer))
			, m_stride(stride)
			, m_bufferSize(buffer_size)
		{
		}

		bool load(ID3D11Device *device) override;
	};

	class FileTexture : public TextureBase
	{
		std::wstring m_filename;
		bool isError;
	public:
		
		FileTexture(std::wstring filename, bool generateMipmaps = true)
			: TextureBase(0, 0, generateMipmaps)
			, m_filename(filename)
			, isError(false)
		{
		}

		bool load(ID3D11Device *device) override;
	};

	class DepthTexture : public Texture
	{
		const DX::DeviceResources &m_deviceResources;

	public:
		DepthTexture(const DX::DeviceResources &deviceResources);
		bool load(ID3D11Device *device) override;
		
	};
}
