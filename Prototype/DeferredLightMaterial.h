﻿#pragma once
#include "Material.h"

namespace OE
{
	class TextureRenderTarget;

	class DeferredLightMaterial : public Material
	{
		std::shared_ptr<Texture> m_color0Texture;
		std::shared_ptr<Texture> m_color1Texture;
		std::shared_ptr<Texture> m_depthTexture;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_color0SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_color1SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_depthSamplerState;

	public:

		const std::shared_ptr<Texture> & getColor0Texture() const { return m_color0Texture; }
		void setColor0Texture(const std::shared_ptr<Texture> &texture) { m_color0Texture = texture; }
		const std::shared_ptr<Texture> & getColor1Texture() const { return m_color1Texture; }
		void setColor1Texture(const std::shared_ptr<Texture> &texture) { m_color1Texture = texture; }
		const std::shared_ptr<Texture> & getDepthTexture() const { return m_depthTexture; }
		void setDepthTexture(const std::shared_ptr<Texture> &texture) { m_depthTexture = texture; }

		void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const override;

		void SetupPointLight(const DirectX::SimpleMath::Vector3 &lightPosition, const DirectX::SimpleMath::Color &color, float getIntensity);
		void SetupDirectionalLight(const DirectX::SimpleMath::Vector3 &lightDirection, const DirectX::SimpleMath::Color &color, float getIntensity);

	protected:

		enum class DeferredLightType : int32_t
		{
			Directional,
			Point
		};

		struct DeferredLightConstants
		{
			DirectX::XMMATRIX invProjection;
			union {
				DirectX::XMFLOAT3 direction;
				DirectX::XMFLOAT3 position;
			};
			DeferredLightType lightType;
			DirectX::XMFLOAT3 intensifiedColor;
		} m_constants;

		UINT inputSlot(VertexAttribute attribute) override;

		ShaderCompileSettings vertexShaderSettings() const override;
		ShaderCompileSettings pixelShaderSettings() const override;

		bool createPSConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) override;
		void updatePSConstantBuffer(const DirectX::SimpleMath::Matrix &worldMatrix, const DirectX::SimpleMath::Matrix &viewMatrix,
			const DirectX::SimpleMath::Matrix &projMatrix, ID3D11DeviceContext *context, ID3D11Buffer *buffer) override;
		void createBlendState(ID3D11Device *device, ID3D11BlendState *&blendState) override;

		void setContextSamplers(const DX::DeviceResources &deviceResources) override;
		void unsetContextSamplers(const DX::DeviceResources &deviceResources) override;
	};
}