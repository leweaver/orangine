#pragma once
#include "OeCore/Material.h"
#include "OeCore/Material_base.h"

namespace oe {
class Clear_gbuffer_material : public Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base> {
  using Base_type = Material_base<Vertex_constant_buffer_empty, Pixel_constant_buffer_base>;

 public:
  Clear_gbuffer_material();

  const std::string& materialType() const override;

  Shader_output_layout getShaderOutputLayout() const override
  {
    static constexpr std::array<Texture_format, 3> gbufferRtvFormats{
            {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT}
    };
    return Shader_output_layout{gbufferRtvFormats};
  }
};
}// namespace oe
