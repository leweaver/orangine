#pragma once

#include "Collision.h"
#include "Texture.h"

namespace oe {
	class Shadow_map_texture : public Texture {
	public:
		const DirectX::BoundingOrientedBox& casterVolume() const { return _boundingOrientedBox; }
		void setCasterVolume(const DirectX::BoundingOrientedBox& boundingOrientedBox) { _boundingOrientedBox = boundingOrientedBox; }

		const DirectX::SimpleMath::Matrix& worldViewProjMatrix() const { return _worldViewProjMatrix; };
		void setWorldViewProjMatrix(const DirectX::SimpleMath::Matrix& worldViewProjMatrix) { _worldViewProjMatrix = worldViewProjMatrix; }

		ID3D11DepthStencilView* depthStencilView() const { return _depthStencilView.Get(); }

		const D3D11_VIEWPORT& viewport() const { return _viewport; }
		
	protected:
		Shadow_map_texture();

		D3D11_VIEWPORT _viewport;
		
		DirectX::BoundingOrientedBox _boundingOrientedBox;
		DirectX::SimpleMath::Matrix _worldViewProjMatrix;
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
		typedef std::function<ID3D11Texture2D*()> Array_texture_source;
		Shadow_map_texture_array_slice(const D3D11_VIEWPORT& viewport, uint32_t arraySlice, Array_texture_source arrayTextureSource);

		void load(ID3D11Device* device) override;
		void unload() override;

		uint32_t arraySlice() const { return _arraySlice; }

	private:
		Array_texture_source _arrayTextureSource;
		uint32_t _arraySlice;
	};
}