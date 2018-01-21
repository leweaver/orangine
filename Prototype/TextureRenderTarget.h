#pragma once

namespace OE
{
	class TextureRenderTarget
	{
	public:
		
		TextureRenderTarget();

		void initialize(ID3D11Device *d3dDevice, int width, int height);
		void shutdown();

		ID3D11RenderTargetView *getRenderTargetView() const { return m_renderTargetView.Get(); }
		ID3D11ShaderResourceView *getShaderResourceView() const { return m_shaderResourceView.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
	};
}
