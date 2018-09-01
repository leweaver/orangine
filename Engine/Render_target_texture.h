#pragma once
#include "Texture.h"

namespace oe
{
	class Render_target_view_texture : public Texture {
	public:
		explicit Render_target_view_texture(ID3D11RenderTargetView* renderTargetView);
		ID3D11RenderTargetView* renderTargetView() const { return _renderTargetView.Get(); }

		void load(ID3D11Device* device) override {};
		void unload() override {};

	protected:
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;
	};

	class Render_target_texture : public Render_target_view_texture
	{
	public:
		static Render_target_texture* createDefaultRgb(uint32_t width, uint32_t height);
		static Render_target_texture* createDefaultShadowMap(uint32_t width, uint32_t height);

		void load(ID3D11Device* device) override;
		void unload() override;

	private:
		explicit Render_target_texture(D3D11_TEXTURE2D_DESC textureDesc);

		D3D11_TEXTURE2D_DESC _textureDesc;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> _renderTargetTexture;
	};
}
