#pragma once
#include "Material.h"
#include "Material_base.h"

namespace oe {
	class Clear_gbuffer_material : public Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base, Vertex_attribute::Position> {
	public:

		const std::string& materialType() const override;
	protected:
		void createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode) override;
	};
}
