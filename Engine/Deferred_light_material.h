#pragma once
#include "Material.h"

namespace oe
{
	class Render_target_texture;

	class Deferred_light_material : public Material
	{
	public:

		Deferred_light_material() = default;

		const std::shared_ptr<Texture>& color0Texture() const { return _color0Texture; }
		void setColor0Texture(const std::shared_ptr<Texture>& texture) { _color0Texture = texture; }
		const std::shared_ptr<Texture>& color1Texture() const { return _color1Texture; }
		void setColor1Texture(const std::shared_ptr<Texture>& texture) { _color1Texture = texture; }
		const std::shared_ptr<Texture>& color2Texture() const { return _color2Texture; }
		void setColor2Texture(const std::shared_ptr<Texture>& texture) { _color2Texture = texture; }
		const std::shared_ptr<Texture>& depthTexture() const { return _depthTexture; }
		void setDepthTexture(const std::shared_ptr<Texture>& texture) { _depthTexture = texture; }
		const std::shared_ptr<Texture>& shadowMapTexture() const { return _shadowMapTexture; }
		void setShadowMapTexture(const std::shared_ptr<Texture>& texture) { _shadowMapTexture = texture; }

		void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const override;

		void setupEmitted(bool enabled);

	protected:

		// This constant buffer contains data that is shared across each light that is rendered, such as the camera data.
		// Individual light parameters are stored in the constant buffer managed by the Render_light_data_impl class.
		struct Deferred_light_material_constants
		{
			DirectX::XMMATRIX viewMatrixInv;
			DirectX::XMMATRIX projMatrixInv;
			DirectX::XMFLOAT4 eyePosition;
			bool emittedEnabled = false;
		} _constants;

		UINT inputSlot(Vertex_attribute attribute) override;

		Shader_compile_settings vertexShaderSettings() const override;
		Shader_compile_settings pixelShaderSettings() const override;

		bool createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) override;
		void updatePSConstantBuffer(const Render_light_data& renderlightData, 
			const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, 
			ID3D11DeviceContext* context, 
			ID3D11Buffer* buffer) override;

		void createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode) override;
		
		void setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData) override;
		void unsetContextSamplers(const DX::DeviceResources& deviceResources) override;

	private:
		std::shared_ptr<Texture> _color0Texture;
		std::shared_ptr<Texture> _color1Texture;
		std::shared_ptr<Texture> _color2Texture;
		std::shared_ptr<Texture> _depthTexture;
		std::shared_ptr<Texture> _shadowMapTexture;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color0SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color1SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color2SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _depthSamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _shadowMapSamplerState;

		int32_t _shadowMapCount = 0;
	};
}
