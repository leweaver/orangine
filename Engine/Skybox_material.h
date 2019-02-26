#pragma once
#include "Material_base.h"

namespace oe {
	using Skybox_material_vs_constant_buffer = Vertex_constant_buffer_base;

	class Skybox_material : public Material_base<Skybox_material_vs_constant_buffer, Pixel_constant_buffer_base, Vertex_attribute::Position> {
        using Base_type = Material_base<Skybox_material_vs_constant_buffer, Pixel_constant_buffer_base, Vertex_attribute::Position>;
	public:
		Skybox_material();

		Skybox_material(const Skybox_material& other) = delete;
		Skybox_material(Skybox_material&& other) = delete;
		void operator=(const Skybox_material& other) = delete;
		void operator=(Skybox_material&& other) = delete;

		virtual ~Skybox_material() = default;

		std::shared_ptr<Texture> cubeMapTexture() const { return _cubeMapTexture; }
		void setCubeMapTexture(std::shared_ptr<Texture> cubeMapTexture) { _cubeMapTexture = cubeMapTexture; }

		const std::string& materialType() const override;

        nlohmann::json serialize(bool compilerPropertiesOnly) const override;
        Shader_resources shaderResources(const Render_light_data& renderLightData) const override;

	private:
		std::shared_ptr<Texture> _cubeMapTexture;
	};
}
