#pragma once

#include "DeviceResources.h"

namespace oe
{
	class Texture {
	public:
		virtual ~Texture() = default;

		bool isValid() const { return _shaderResourceView != nullptr; }

		ID3D11ShaderResourceView* getShaderResourceView() const { return _shaderResourceView.Get(); }

		// Will throw a std::exception if texture failed to load.
		virtual void load(ID3D11Device* device) = 0;
		virtual void unload();

	protected:
		Texture()
			: _shaderResourceView(nullptr)
		{}
		explicit Texture(ID3D11ShaderResourceView* srv)
			: _shaderResourceView(srv)
		{}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
	};

	class Buffer_texture : public Texture
	{
	public:

		Buffer_texture(uint32_t stride, uint32_t buffer_size, std::unique_ptr<uint8_t>& buffer)
			: _stride(stride)
			, _bufferSize(buffer_size)
			, _buffer(std::move(buffer))
		{
		}

		void load(ID3D11Device* device) override;

	private:
		uint32_t _stride;
		uint32_t _bufferSize;
		std::unique_ptr<uint8_t> _buffer;
	};

	class File_texture : public Texture
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> _textureResource;
		std::wstring _filename;
	public:

		explicit File_texture(std::wstring&& filename)
			: _filename(std::move(filename))
		{
		}

		void load(ID3D11Device* device) override;
		void unload() override;
		const std::wstring& filename() const { return _filename; }
	};

	class Depth_texture : public Texture
	{
		const DX::DeviceResources& _deviceResources;

	public:
		explicit Depth_texture(const DX::DeviceResources& deviceResources);
		void load(ID3D11Device* device) override;
	};
}
