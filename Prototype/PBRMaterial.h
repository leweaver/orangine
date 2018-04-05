#pragma once
#include "Material.h"
#include "MaterialRepository.h"
#include "Texture.h"
#include <memory>

namespace OE {
	class PBRMaterial : public Material
	{
		DirectX::SimpleMath::Color m_baseColor;

		enum TextureType
		{
			BaseColor,
			MetallicRoughness,
			Normal,
			Occlusion,
			Emissive,

			NumTextureTypes
		};

		std::shared_ptr<Texture> m_textures[NumTextureTypes];

		// Compiled state
		unsigned int m_boundTextureCount;
		ID3D11SamplerState *m_samplerStates[NumTextureTypes];
		ID3D11ShaderResourceView *m_shaderResourceViews[NumTextureTypes];

	public:
		PBRMaterial();
		virtual ~PBRMaterial();

		DirectX::SimpleMath::Color getBaseColor() const 
		{
			return m_baseColor;
		}
		void setBaseColor(const DirectX::SimpleMath::Color &color)
		{
			m_baseColor = color;
		}

		const std::shared_ptr<Texture> &getBaseColorTexture() const
		{
			return m_textures[BaseColor];
		}

		void setBaseColorTexture(const std::shared_ptr<Texture> &baseColorTexture)
		{
			if (m_textures[BaseColor] != baseColorTexture) {
				m_textures[BaseColor] = baseColorTexture;
				markRequiresRecomplie();
			}
		}

		const std::shared_ptr<Texture> &getMetallicRoughnessTexture() const
		{
			return m_textures[MetallicRoughness];
		}

		void setMetallicRoughnessTexture(const std::shared_ptr<Texture> &metallicRoughnessTexture)
		{
			if (m_textures[MetallicRoughness] != metallicRoughnessTexture) {
				m_textures[MetallicRoughness] = metallicRoughnessTexture;
				markRequiresRecomplie();
			}
		}

		const std::shared_ptr<Texture> &getNormalTexture() const
		{
			return m_textures[Normal];
		}

		void setNormalTexture(const std::shared_ptr<Texture> &normalTexture)
		{
			if (m_textures[Normal] != normalTexture) {
				m_textures[Normal] = normalTexture;
				markRequiresRecomplie();
			}
		}

		const std::shared_ptr<Texture> &getOcclusionTexture() const
		{
			return m_textures[Occlusion];
		}

		void setOcclusionTexture(const std::shared_ptr<Texture> &occlusionTexture)
		{
			if (m_textures[Occlusion] != occlusionTexture) {
				m_textures[Occlusion] = occlusionTexture;
				markRequiresRecomplie();
			}
		}

		const std::shared_ptr<Texture> &getEmissiveTexture() const
		{
			return m_textures[Emissive];
		}

		void setEmissiveTexture(const std::shared_ptr<Texture> &emissiveTexture)
		{
			if (m_textures[Emissive] != emissiveTexture) {
				m_textures[Emissive] = emissiveTexture;
				markRequiresRecomplie();
			}
		}

		void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const override;

	protected:
		
		struct PBRConstantsVS
		{
			DirectX::XMMATRIX worldViewProjection;
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX worldInvTranspose;
		} m_constantsVS{};
		struct PBRConstantsPS
		{
			DirectX::XMMATRIX world;
			DirectX::XMFLOAT4 baseColor;
		} m_constantsPS{};

		UINT inputSlot(VertexAttribute attribute) override;

		ShaderCompileSettings vertexShaderSettings() const override;
		ShaderCompileSettings pixelShaderSettings() const override;

		bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer *&buffer) override;
		void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context, 
			ID3D11Buffer *buffer) override;

		bool createPSConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) override;
		void updatePSConstantBuffer(const DirectX::SimpleMath::Matrix &worldMatrix,
			const DirectX::SimpleMath::Matrix &viewMatrix,
			const DirectX::SimpleMath::Matrix &projMatrix,
			ID3D11DeviceContext *context,
			ID3D11Buffer *buffer) override;

		void createShaderResources(const DX::DeviceResources &deviceResources);
		void releaseBindings();

		void setContextSamplers(const DX::DeviceResources &deviceResources) override;
		void unsetContextSamplers(const DX::DeviceResources &deviceResources) override;
	};

	template <>
	inline std::unique_ptr<PBRMaterial> MaterialRepository::instantiate() const
	{
		return std::make_unique<PBRMaterial>();
	}
}
