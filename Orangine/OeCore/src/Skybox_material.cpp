#include "OeCore/Skybox_material.h"
#include "OeCore/Texture.h"

using namespace oe;

const std::string g_material_type = "Skybox_material";
const std::string g_json_cubeMapTexture = "cube_map_texture";

// Uses blended alpha, to ensure standard rendering pipeline (not deferred)
Skybox_material::Skybox_material()
    : Material(
              static_cast<uint8_t>(Material_type_index::Skybox), Material_alpha_mode::Opaque,
              Material_face_cull_mode::Front_face)
{}

const std::string& Skybox_material::materialType() const { return g_material_type; }

nlohmann::json Skybox_material::serialize(bool compilerPropertiesOnly) const
{
  auto j = Material::serialize(compilerPropertiesOnly);
  j[g_json_cubeMapTexture] = serializeTexture(compilerPropertiesOnly, _cubeMapTexture);

  return j;
}

Material::Shader_resources
Skybox_material::shaderResources(const std::set<std::string>& flags, const Render_light_data& renderLightData) const
{
  auto sr = Material::shaderResources(flags, renderLightData);
  if (_cubeMapTexture) {
    sr.textures.push_back({_cubeMapTexture});

    auto samplerDesc = Sampler_descriptor();
    samplerDesc.wrapU = Sampler_texture_address_mode::Wrap;
    samplerDesc.wrapV = Sampler_texture_address_mode::Wrap;
    samplerDesc.wrapW = Sampler_texture_address_mode::Wrap;
    sr.samplerDescriptors.push_back(samplerDesc);
  }
  return sr;
}

void Skybox_material::updatePerDrawConstantBuffer(
        gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
        const Update_constant_buffer_inputs& inputs)
{
  OE_CHECK(
          bufferDesc.getRegisterIndex() == kCbRegisterVsMain &&
          bufferDesc.getVisibility() == Shader_constant_buffer_visibility::Vertex);
  OE_CHECK(cpuBuffer.size() >= sizeof(Skybox_material_vs_constant_buffer));

  auto* cb = reinterpret_cast<Skybox_material_vs_constant_buffer*>(cpuBuffer.data());
  cb->worldViewProjection = inputs.projectionMatrix * inputs.viewMatrix * inputs.worldTransform;
}

Shader_constant_layout Skybox_material::getShaderConstantLayout() const
{
  static std::array<Shader_layout_constant_buffer, 1> layout{{
          Shader_layout_constant_buffer{
                  kCbRegisterVsMain, Shader_constant_buffer_visibility::Vertex,
                  Shader_layout_constant_buffer::Usage_per_draw_data{sizeof(Skybox_material_vs_constant_buffer)}},
  }};
  return Shader_constant_layout{layout};
}
