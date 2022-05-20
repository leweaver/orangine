#pragma once

#include <OeCore/Material.h>

namespace oe {
class Clear_gbuffer_material : public Material {
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

  void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) override;

  Shader_constant_layout getShaderConstantLayout() const override;
};
}// namespace oe
