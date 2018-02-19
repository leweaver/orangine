#pragma once
#include "Texture.h"

namespace OE
{
	class TextureRenderTarget : public Texture
	{
		uint32_t m_width;
		uint32_t m_height;
	public:
		
		TextureRenderTarget(uint32_t width, uint32_t height);

		void load(ID3D11Device *device) override;
		void unload() override;

		ID3D11RenderTargetView *getRenderTargetView() const { return m_renderTargetView.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	};
}
