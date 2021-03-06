#pragma once

#include "OeCore/Material_base.h"
#include "OeCore/Color.h"
#include "Renderer_types.h"

namespace oe {
	struct Unlit_material_vs_constant_buffer : Vertex_constant_buffer_base {
		Float4 baseColor;
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

		const Color& baseColor() const { return _baseColor; }
		void setBaseColor(const Color& baseColor) { _baseColor = baseColor; }
		
		const std::string& materialType() const override;

        nlohmann::json serialize(bool compilerPropertiesOnly) const override;
        
        std::set<std::string> configFlags(
            const Renderer_features_enabled& rendererFeatures,
            Render_pass_blend_mode blendMode,
            const Mesh_vertex_layout& meshVertexLayout) const override;
        std::vector<Vertex_attribute_element> vertexInputs(const std::set<std::string>& flags) const override;
        Material::Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override;

	protected:
		
		void updateVsConstantBufferValues(Unlit_material_vs_constant_buffer& constants, 
			const SSE::Matrix4& worldMatrix,
			const SSE::Matrix4& viewMatrix,
			const SSE::Matrix4& projMatrix,
            const Renderer_animation_data& rendererAnimationData) const override;

	private:
		Color _baseColor;
	};
}
