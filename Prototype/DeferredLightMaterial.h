#pragma once
#include "Material.h"

namespace OE
{
	class TextureRenderTarget;

	class DeferredLightMaterial : public Material
	{
		std::shared_ptr<TextureRenderTarget> m_color0Texture;
		std::shared_ptr<TextureRenderTarget> m_color1Texture;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_color0SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_color1SamplerState;

	public:

		const std::shared_ptr<TextureRenderTarget> & getColor0Texture() const
		{
			return m_color0Texture;
		}

		void setColor0Texture(const std::shared_ptr<TextureRenderTarget> &textureRenderTarget)
		{
			m_color0Texture = textureRenderTarget;
		}

		const std::shared_ptr<TextureRenderTarget> & getColor1Texture() const
		{
			return m_color1Texture;
		}

		void setColor1Texture(const std::shared_ptr<TextureRenderTarget> &textureRenderTarget)
		{
			m_color1Texture = textureRenderTarget;
		}

		void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const override;

	protected:
		UINT inputSlot(VertexAttribute attribute) override;

		ShaderCompileSettings vertexShaderSettings() const override;
		ShaderCompileSettings pixelShaderSettings() const override;

		bool createConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) override;
		void updateConstantBuffer(const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &viewMatrix,
			const DirectX::XMMATRIX &projMatrix, ID3D11DeviceContext *context, ID3D11Buffer *buffer) override;
		void setContextSamplers(const DX::DeviceResources &deviceResources) override;
		void unsetContextSamplers(const DX::DeviceResources &deviceResources) override;
	};
}
