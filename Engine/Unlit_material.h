#pragma once

#include "Material_base.h"

namespace oe {
	struct Unlit_material_vs_constant_buffer : Vertex_constant_buffer_base {
		DirectX::SimpleMath::Color baseColor;
	};

	class Unlit_material : public Material_base<Unlit_material_vs_constant_buffer, Pixel_constant_buffer_base> {
        using Base_type = Material_base<Unlit_material_vs_constant_buffer, Pixel_constant_buffer_base>;
	public:
		Unlit_material();

		Unlit_material(const Unlit_material& other) = delete;
		Unlit_material(Unlit_material&& other) = delete;
		void operator=(const Unlit_material& other) = delete;
		void operator=(Unlit_material&& other) = delete;

		virtual ~Unlit_material() = default;

		DirectX::SimpleMath::Color baseColor() const { return _baseColor; }
		void setBaseColor(const DirectX::SimpleMath::Color& baseColor) { _baseColor = baseColor; }
		
		const std::string& materialType() const override;

        nlohmann::json serialize(bool compilerPropertiesOnly) const override;

	protected:
		
		void updateVSConstantBufferValues(Unlit_material_vs_constant_buffer& constants, 
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
            const Renderer_animation_data& rendererAnimationData) const override;

	private:
		DirectX::SimpleMath::Color _baseColor;
	};
}
