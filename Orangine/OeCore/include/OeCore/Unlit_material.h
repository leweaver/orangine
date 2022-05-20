#pragma once

#include <OeCore/Color.h>
#include <OeCore/Material.h>
#include "Renderer_types.h"

namespace oe {
__declspec(align(16)) struct Unlit_material_vs_constant_buffer {
  SSE::Matrix4 worldViewProjection;
  Float4 baseColor;
};

class Unlit_material : public Material {
 public:
  inline static constexpr uint32_t kCbRegisterVsMain = 0;

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
          const Renderer_features_enabled& rendererFeatures, Render_pass_blend_mode blendMode,
          const Mesh_vertex_layout& meshVertexLayout) const override;
  std::vector<Vertex_attribute_element> vertexInputs(const std::set<std::string>& flags) const override;
  Material::Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override;

  void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) override;

  Shader_constant_layout getShaderConstantLayout() const override;

 protected:
  void updateVsConstantBufferValues(
          Unlit_material_vs_constant_buffer& constants, const SSE::Matrix4& worldMatrix, const SSE::Matrix4& viewMatrix,
          const SSE::Matrix4& projMatrix, const Renderer_animation_data& rendererAnimationData) const;

 private:
  Color _baseColor;
};
}// namespace oe
