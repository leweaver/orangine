#pragma once

#include "Texture.h"
#include "DirectXTex.h"
#include "DeviceResources.h"

namespace OE {
	class BufferTexture : public Texture
	{
		std::unique_ptr<uint8_t> m_buffer;

		uint32_t m_stride;
		uint32_t m_bufferSize;
	public:

		BufferTexture(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t> &buffer)
			: m_buffer(std::move(buffer))
			, m_stride(stride)
			, m_bufferSize(buffer_size)
		{
		}

		void load(ID3D11Device *device) override;
	};

	class FileTexture : public Texture
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> m_textureResource;
		std::wstring m_filename;
	public:
		
		FileTexture(std::wstring filename)
			: m_filename(filename)
		{
		}

		void load(ID3D11Device *device) override;
		void unload() override;
	};

	class DepthTexture : public Texture
	{
		const DX::DeviceResources &m_deviceResources;

	public:
		DepthTexture(const DX::DeviceResources &deviceResources);
		void load(ID3D11Device *device) override;
		
	};
}
