#pragma once

namespace OE
{
	class Texture
	{
	protected:
		Texture(uint32_t width, uint32_t height)
			: m_shaderResourceView(nullptr)
			, m_width(width)
			, m_height(height)
		{}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
		uint32_t m_width;
		uint32_t m_height;

	public:
		virtual ~Texture();

		bool isValid() const { return m_shaderResourceView != nullptr; }

		ID3D11ShaderResourceView *getShaderResourceView() const { return m_shaderResourceView.Get(); }
		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }

		virtual bool load(ID3D11Device *device) = 0;
		virtual void unload();
	};
}
