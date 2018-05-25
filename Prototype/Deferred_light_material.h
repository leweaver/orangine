#pragma once
#include "Material.h"

namespace oe
{
	class Render_target_texture;

	class Deferred_light_material : public Material
	{
	public:

		const std::shared_ptr<Texture>& getColor0Texture() const { return _color0Texture; }
		void setColor0Texture(const std::shared_ptr<Texture>& texture) { _color0Texture = texture; }
		const std::shared_ptr<Texture>& getColor1Texture() const { return _color1Texture; }
		void setColor1Texture(const std::shared_ptr<Texture>& texture) { _color1Texture = texture; }
		const std::shared_ptr<Texture>& getColor2Texture() const { return _color2Texture; }
		void setColor2Texture(const std::shared_ptr<Texture>& texture) { _color2Texture = texture; }
		const std::shared_ptr<Texture>& getDepthTexture() const { return _depthTexture; }
		void setDepthTexture(const std::shared_ptr<Texture>& texture) { _depthTexture = texture; }

		void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const override;

		void setupPointLight(const DirectX::SimpleMath::Vector3& lightPosition, const DirectX::SimpleMath::Color& color, float getIntensity);
		void setupDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, const DirectX::SimpleMath::Color& color, float getIntensity);
		void setupAmbientLight(const DirectX::SimpleMath::Color& color, float getIntensity);
		void setupEmitted();

	protected:

		enum class Deferred_light_type : int32_t
		{
			Directional,
			Point,
			Ambient,
			Emitted
		};

		struct Deferred_light_constants
		{
			DirectX::XMMATRIX invProjection;
			union {
				DirectX::XMFLOAT3 direction;
				DirectX::XMFLOAT3 position;
			};
			Deferred_light_type lightType;
			DirectX::XMFLOAT4 eyePosition;
			DirectX::XMFLOAT3 intensifiedColor;
		} _constants;

		UINT inputSlot(Vertex_attribute attribute) override;

		Shader_compile_settings vertexShaderSettings() const override;
		Shader_compile_settings pixelShaderSettings() const override;

		bool createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) override;
		void updatePSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, ID3D11DeviceContext* context, ID3D11Buffer* buffer) override;
		void createBlendState(ID3D11Device* device, ID3D11BlendState*& blendState) override;

		void setContextSamplers(const DX::DeviceResources& deviceResources) override;
		void unsetContextSamplers(const DX::DeviceResources& deviceResources) override;

	private:
		std::shared_ptr<Texture> _color0Texture;
		std::shared_ptr<Texture> _color1Texture;
		std::shared_ptr<Texture> _color2Texture;
		std::shared_ptr<Texture> _depthTexture;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color0SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color1SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _color2SamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _depthSamplerState;
	};
}
