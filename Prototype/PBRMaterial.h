#pragma once
#include "Material.h"
#include "MaterialRepository.h"
#include "Texture.h"
#include <memory>

namespace OE {
	class PBRMaterial : public Material
	{
		DirectX::SimpleMath::Color m_baseColor;

		std::shared_ptr<Texture> m_baseColorTexture;
		std::shared_ptr<Texture> m_metallicRoughnessTexture;
		std::shared_ptr<Texture> m_normalTexture;
		std::shared_ptr<Texture> m_occlusionTexture;
		std::shared_ptr<Texture> m_emissiveTexture;

		ID3D11SamplerState* m_sampleState;

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
			return m_baseColorTexture;
		}

		void setBaseColorTexture(const std::shared_ptr<Texture> &baseColorTexture)
		{
			m_baseColorTexture = baseColorTexture;
		}

		const std::shared_ptr<Texture> &getMetallicRoughnessTexture() const
		{
			return m_metallicRoughnessTexture;
		}

		void setMetallicRoughnessTexture(const std::shared_ptr<Texture> &metallicRoughnessTexture)
		{
			m_metallicRoughnessTexture = metallicRoughnessTexture;
		}

		const std::shared_ptr<Texture> &getNormalTexture() const
		{
			return m_normalTexture;
		}

		void setNormalTexture(const std::shared_ptr<Texture> &normalTexture)
		{
			m_normalTexture = normalTexture;
		}

		const std::shared_ptr<Texture> &getOcclusionTexture() const
		{
			return m_occlusionTexture;
		}

		void setOcclusionTexture(const std::shared_ptr<Texture> &occlusionTexture)
		{
			m_occlusionTexture = occlusionTexture;
		}

		const std::shared_ptr<Texture> &getEmissiveTexture() const
		{
			return m_emissiveTexture;
		}

		void setEmissiveTexture(const std::shared_ptr<Texture> &emissiveTexture)
		{
			m_emissiveTexture = emissiveTexture;
		}

		void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const override;

	protected:
		
		struct PBRConstants
		{
			DirectX::XMMATRIX viewProjection;
			DirectX::XMMATRIX world;
			DirectX::XMFLOAT4 baseColor;
		} m_constants;

		UINT inputSlot(VertexAttribute attribute) override;

		ShaderCompileSettings vertexShaderSettings() const override;
		ShaderCompileSettings pixelShaderSettings() const override;

		bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer *&buffer) override;
		void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context, 
			ID3D11Buffer *buffer) override;

		void setContextSamplers(const DX::DeviceResources &deviceResources) override;
		void unsetContextSamplers(const DX::DeviceResources &deviceResources) override;
	};

	template <>
	inline std::unique_ptr<PBRMaterial> MaterialRepository::instantiate() const
	{
		return std::make_unique<PBRMaterial>();
	}
}
