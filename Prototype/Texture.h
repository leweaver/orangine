#pragma once

namespace OE
{
	class Texture
	{
	protected:
		Texture()
			: m_shaderResourceView(nullptr)
		{}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;

	public:
		virtual ~Texture() = default;

		bool isValid() const { return m_shaderResourceView != nullptr; }

		ID3D11ShaderResourceView *getShaderResourceView() const { return m_shaderResourceView.Get(); }

		// Will throw a std::exception if texture failed to load.
		virtual void load(ID3D11Device *device) = 0;

		virtual void unload();
	};
}
