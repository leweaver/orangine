#pragma once
#include "Material_base.h"

namespace oe {
	using Skybox_material_vs_constant_buffer = Vertex_constant_buffer_base;

	class Skybox_material : public Material_base<Skybox_material_vs_constant_buffer, Pixel_constant_buffer_base, Vertex_attribute::Position> {
	public:
		Skybox_material();

		Skybox_material(const Skybox_material& other) = delete;
		Skybox_material(Skybox_material&& other) = delete;
		void operator=(const Skybox_material& other) = delete;
		void operator=(Skybox_material&& other) = delete;

		virtual ~Skybox_material() = default;

		std::shared_ptr<Texture> cubemapTexture() const { return _cubemapTexture; }
		void setCubemapTexture(std::shared_ptr<Texture> cubemapTexture) { _cubemapTexture = cubemapTexture; }

		const std::string& materialType() const override;
		void releaseShaderResources();
		void createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData,
		                           Render_pass_blend_mode blendMode) override;

		void setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData) override;
		void unsetContextSamplers(const DX::DeviceResources& deviceResources) override;

	private:
		std::shared_ptr<Texture> _cubemapTexture;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState;

	};
}
