#pragma once

#include "Collision.h"
#include "Texture.h"

namespace oe {
	class Shadow_map_texture : public Texture {
	public:
		Shadow_map_texture(uint32_t width, uint32_t height);

		const DirectX::BoundingOrientedBox& casterVolume() const { return _boundingOrientedBox; }
		void setCasterVolume(const DirectX::BoundingOrientedBox& boundingOrientedBox) { _boundingOrientedBox = boundingOrientedBox; }

		ID3D11DepthStencilView* depthStencilView() const { return _depthStencilView.Get(); }

		uint32_t width() const { return _width; };
		uint32_t height() const { return _height; };

		// Will throw a std::exception if texture failed to load.
		void load(ID3D11Device* device) override;
		void unload() override;

	private:
		uint32_t _width;
		uint32_t _height;
		
		DirectX::BoundingOrientedBox _boundingOrientedBox;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthStencilView;
	};
}