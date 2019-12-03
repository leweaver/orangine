#pragma once

#include "Collision.h"
#include "Texture.h"

namespace oe {
	class Shadow_map_texture : public Texture {
	public:
		const DirectX::BoundingOrientedBox& casterVolume() const { return _boundingOrientedBox; }
		void setCasterVolume(const DirectX::BoundingOrientedBox& boundingOrientedBox) { _boundingOrientedBox = boundingOrientedBox; }

		const SSE::Matrix4& worldViewProjMatrix() const { return _worldViewProjMatrix; };
		void setWorldViewProjMatrix(const SSE::Matrix4& worldViewProjMatrix) { _worldViewProjMatrix = worldViewProjMatrix; }

		ID3D11DepthStencilView* depthStencilView() const { return _depthStencilView.Get(); }

		const D3D11_VIEWPORT& viewport() const { return _viewport; }
		
	protected:
		explicit Shadow_map_texture(const D3D11_VIEWPORT& viewport);

		D3D11_VIEWPORT _viewport;
		
		DirectX::BoundingOrientedBox _boundingOrientedBox;
		SSE::Matrix4 _worldViewProjMatrix;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthStencilView;
	};

	class Shadow_map_texture_basic : public Shadow_map_texture {
	public:
		Shadow_map_texture_basic(uint32_t width, uint32_t height);

		uint32_t width() const { return _width; };
		uint32_t height() const { return _height; };

		void load(ID3D11Device* device) override;
		void unload() override;

	protected:
		uint32_t _width;
		uint32_t _height;
	};

	class Shadow_map_texture_array_slice : public Shadow_map_texture {
	public:
		struct Array_texture {
			ID3D11Texture2D* texture;
			ID3D11ShaderResourceView* srv;
			uint32_t arraySize;
		} ;

		typedef std::function<Array_texture()> Array_texture_source;

		Shadow_map_texture_array_slice(const D3D11_VIEWPORT& viewport, uint32_t arraySlice, uint32_t dimension, Array_texture_source arrayTextureSource);

		void load(ID3D11Device* device) override;
		void unload() override;

		uint32_t arraySlice() const { return _arraySlice; }
		uint32_t textureWidth() const {
			return _textureWidth;
		}

	private:
		Array_texture_source _arrayTextureSource;
		uint32_t _arraySlice;
		uint32_t _textureWidth;
	};
}