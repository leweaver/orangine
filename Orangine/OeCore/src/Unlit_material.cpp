#include "OeCore/JsonUtils.h"
#include "OeCore/Unlit_material.h"

using namespace std::string_literals;
using namespace DirectX;
using namespace oe;

const std::string g_material_type = "Unlit_material";

const std::string g_json_baseColor = "base_color";

const std::string g_flag_enableVertexColor = "enable_vertex_color";

// Uses blended alpha, to ensure standard rendering pipeline (not deferred)
Unlit_material::Unlit_material()
    : Material(static_cast<uint8_t>(Material_type_index::Unlit), Material_alpha_mode::Blend),
      _baseColor(Colors::White)
{
}

const std::string& Unlit_material::materialType() const { return g_material_type; }

nlohmann::json Unlit_material::serialize(bool compilerPropertiesOnly) const
{
  auto j = Material::serialize(compilerPropertiesOnly);

  if (!compilerPropertiesOnly) {
    j[g_json_baseColor] = _baseColor;
  }

  return j;
}

std::set<std::string> Unlit_material::configFlags(const Renderer_features_enabled& rendererFeatures,
                                                  Render_pass_blend_mode blendMode,
                                                  const Mesh_vertex_layout& meshVertexLayout) const
{
  auto flags = Material::configFlags(rendererFeatures, blendMode, meshVertexLayout);

  const auto vertexLayout = meshVertexLayout.vertexLayout();
  const auto hasColors = std::any_of(vertexLayout.begin(), vertexLayout.end(), [](const auto& vae) {
    return vae.semantic == Vertex_attribute_semantic{Vertex_attribute::Color, 0};
  });
  if (hasColors) {
    flags.insert(g_flag_enableVertexColor);
  }

  return flags;
}

void Unlit_material::updatePerDrawConstantBuffer(
        gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
        const Update_constant_buffer_inputs& inputs)
{
  OE_CHECK(
          bufferDesc.getRegisterIndex() == kCbRegisterVsMain &&
          bufferDesc.getVisibility() == Shader_constant_buffer_visibility::Vertex);
  OE_CHECK(cpuBuffer.size() >= sizeof(Unlit_material_vs_constant_buffer));

  auto* cb = reinterpret_cast<Unlit_material_vs_constant_buffer*>(cpuBuffer.data());
  cb->worldViewProjection = inputs.projectionMatrix * inputs.viewMatrix * inputs.worldTransform;
  cb->baseColor = static_cast<Float4>(_baseColor);
}

Shader_constant_layout Unlit_material::getShaderConstantLayout() const
{
  static std::array<Shader_layout_constant_buffer, 1> layout{{
          Shader_layout_constant_buffer{
                  kCbRegisterVsMain, Shader_constant_buffer_visibility::Vertex,
                  Shader_layout_constant_buffer::Usage_per_draw_data{sizeof(Unlit_material_vs_constant_buffer)}},
  }};
  return Shader_constant_layout{layout};
}

std::vector<Vertex_attribute_element>
Unlit_material::vertexInputs(const std::set<std::string>& flags) const
{
  auto vertexAttributes = Material::vertexInputs(flags);

  if (flags.find(g_flag_enableVertexColor) != flags.end()) {
    vertexAttributes.push_back(
        {{Vertex_attribute::Color, 0}, Element_type::Vector4, Element_component::Float});
  }

  return vertexAttributes;
}

Material::Shader_compile_settings
Unlit_material::vertexShaderSettings(const std::set<std::string>& flags) const
{
  auto settings = Material::vertexShaderSettings(flags);

  if (flags.find(g_flag_enableVertexColor) != flags.end()) {
    settings.defines["VB_VERTEX_COLORS"] = "1";
  }

  return settings;
}
