#pragma once
#include "Material.h"
#include "Material_base.h"

namespace oe {
	class Clear_gbuffer_material : public Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base, Vertex_attribute::Position> {
        using Base_type = Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base, Vertex_attribute::Position>;
	public:

        Clear_gbuffer_material();

		const std::string& materialType() const override;
	};
}
