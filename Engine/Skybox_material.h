#pragma once
#include "Material_base.h"

namespace oe {
	struct Skybox_material_vs_constant_buffer : Vertex_constant_buffer_base {
		DirectX::SimpleMath::Matrix worldViewProjection;
	};

	class Skybox_material : public Material_base<Skybox_material_vs_constant_buffer, Pixel_constant_buffer_base, Vertex_attribute::Position> {
	public:
		Skybox_material() = default;

		Skybox_material(const Skybox_material& other) = delete;
		Skybox_material(Skybox_material&& other) = delete;
		void operator=(const Skybox_material& other) = delete;
		void operator=(Skybox_material&& other) = delete;

		virtual ~Skybox_material() = default;

		std::shared_ptr<Texture> cubemapTexture() const { return _cubemapTexture; }
		void setCubemapTexture(std::shared_ptr<Texture> cubemapTexture) { _cubemapTexture = cubemapTexture; }

		const std::string& materialType() const override;

	private:
		std::shared_ptr<Texture> _cubemapTexture;

	};
}
