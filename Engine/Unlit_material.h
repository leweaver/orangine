#pragma once

#include "Material.h"

namespace oe {
	class Unlit_material : public Material {
	public:
		Unlit_material();

		Unlit_material(const Unlit_material& other) = delete;
		Unlit_material(Unlit_material&& other) = delete;
		void operator=(const Unlit_material& other) = delete;
		void operator=(Unlit_material&& other) = delete;

		virtual ~Unlit_material() = default;

		DirectX::SimpleMath::Color baseColor() const { return _baseColor; }
		void setBaseColor(const DirectX::SimpleMath::Color& baseColor) { _baseColor = baseColor; }

		void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const override;

	protected:
		struct Unlit_constants_vs {
			DirectX::SimpleMath::Matrix worldViewProjection;
			DirectX::SimpleMath::Color baseColor;
		};

		uint32_t inputSlot(Vertex_attribute attribute) override;

		Shader_compile_settings vertexShaderSettings() const override;
		Shader_compile_settings pixelShaderSettings() const override;

		bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) override;
		void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context,
			ID3D11Buffer* buffer) override;

		void createShaderResources(const DX::DeviceResources& deviceResources, Render_pass_output_format outputFormat) override;

	private:
		DirectX::SimpleMath::Color _baseColor;
		Unlit_constants_vs _constantsVs;
	};
}
