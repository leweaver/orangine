#pragma once
#include "OeCore/Material.h"
#include "OeCore/Material_base.h"

namespace oe {
	class Clear_gbuffer_material : public Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base> {
        using Base_type = Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base>;
	public:

        Clear_gbuffer_material();

		const std::string& materialType() const override;
	};
}
