#pragma once
#include "Texture.h"

namespace oe
{
	class Render_target_texture : public Texture
	{
	public:
		
		Render_target_texture(uint32_t width, uint32_t height);

		void load(ID3D11Device* device) override;
		void unload() override;

		ID3D11RenderTargetView* renderTargetView() const { return _renderTargetView.Get(); }

	private:
		uint32_t _width;
		uint32_t _height;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> _renderTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;
	};
}
