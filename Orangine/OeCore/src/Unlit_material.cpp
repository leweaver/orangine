#include "pch.h"

#include "OeCore/JsonUtils.h"
#include "OeCore/Material_repository.h"
#include "OeCore/Unlit_material.h"

using namespace std::string_literals;
using namespace DirectX;
using namespace oe;

const std::string g_material_type = "Unlit_material";

const std::string g_json_baseColor = "base_color";

const std::string g_flag_enableVertexColor = "enable_vertex_color";

// Uses blended alpha, to ensure standard rendering pipeline (not deferred)
Unlit_material::Unlit_material()
    : Base_type(static_cast<uint8_t>(Material_type_index::Unlit), Material_alpha_mode::Blend),
      _baseColor(Colors::White)
{
}

const std::string& Unlit_material::materialType() const { return g_material_type; }

nlohmann::json Unlit_material::serialize(bool compilerPropertiesOnly) const
{
  auto j = Base_type::serialize(compilerPropertiesOnly);

  if (!compilerPropertiesOnly) {
    j[g_json_baseColor] = _baseColor;
  }

  return j;
}

std::set<std::string> Unlit_material::configFlags(const Renderer_features_enabled& rendererFeatures,
                                                  Render_pass_blend_mode blendMode,
                                                  const Mesh_vertex_layout& meshVertexLayout) const
{
  auto flags = Base_type::configFlags(rendererFeatures, blendMode, meshVertexLayout);

  const auto vertexLayout = meshVertexLayout.vertexLayout();
  const auto hasColors = std::any_of(vertexLayout.begin(), vertexLayout.end(), [](const auto& vae) {
    return vae.semantic == Vertex_attribute_semantic{Vertex_attribute::Color, 0};
  });
  if (hasColors) {
    flags.insert(g_flag_enableVertexColor);
  }

  return flags;
}

void Unlit_material::updateVSConstantBufferValues(
    Unlit_material_vs_constant_buffer& constants, const SSE::Matrix4& /* worldMatrix */,
    const SSE::Matrix4& /* viewMatrix */, const SSE::Matrix4& /* projMatrix */,
    const Renderer_animation_data& /* rendererAnimationData */) const
{
  constants.baseColor = static_cast<Float4>(_baseColor);
}

std::vector<Vertex_attribute_element>
Unlit_material::vertexInputs(const std::set<std::string>& flags) const
{
  auto vertexAttributes = Base_type::vertexInputs(flags);

  if (flags.find(g_flag_enableVertexColor) != flags.end()) {
    vertexAttributes.push_back(
        {{Vertex_attribute::Color, 0}, Element_type::Vector4, Element_component::Float});
  }

  return vertexAttributes;
}

Material::Shader_compile_settings
Unlit_material::vertexShaderSettings(const std::set<std::string>& flags) const
{
  auto settings = Base_type::vertexShaderSettings(flags);

  if (flags.find(g_flag_enableVertexColor) != flags.end()) {
    settings.defines["VB_VERTEX_COLORS"] = "1";
  }

  return settings;
}
