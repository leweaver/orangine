#pragma once
#include "Material.h"

namespace oe {
	class Clear_gbuffer_material : public Material
	{
	public:

		void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const override;

	protected:
		uint32_t inputSlot(Vertex_attribute attribute) override;

		Shader_compile_settings vertexShaderSettings() const override;
		Shader_compile_settings pixelShaderSettings() const override;
	};
}
