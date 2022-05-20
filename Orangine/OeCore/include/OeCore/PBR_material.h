#pragma once

#include "Material.h"
#include "Texture.h"

#include "Color.h"
#include "Renderer_types.h"
#include <array>
#include <memory>

namespace oe {
__declspec(align(16))
struct PBR_material_vs_constant_buffer {
  SSE::Matrix4 worldViewProjection;
  SSE::Matrix4 world;
  SSE::Matrix4 viewProjection;
  SSE::Matrix4 worldInvTranspose;
  Float4 morphWeights[2];
};

__declspec(align(16))
struct PBR_material_ps_constant_buffer {
  SSE::Matrix4 world;
  Float4 baseColor;
  Float4 metallicRoughness; // metallic, roughness
  Float4 emissive;          // emissive color (RGB)
};

class PBR_material : public Material {
  enum Texture_type {
    BaseColor,
    MetallicRoughness,
    Normal,
    Occlusion,
    Emissive,

    NumTextureTypes
  };

 public:
  inline static constexpr uint32_t kCbRegisterVsMain = 0;
  inline static constexpr uint32_t kCbRegisterPsMain = 1;
  inline static constexpr uint32_t kCbRegisterVsSkinning = 2;

  PBR_material();

  PBR_material(const PBR_material& other) = delete;
  PBR_material(PBR_material&& other) = delete;
  void operator=(const PBR_material& other) = delete;
  void operator=(PBR_material&& other) = delete;

  /*
   * The RGBA components of the base color of the material.The fourth component(A) is the alpha
   * coverage of the material. The alphaMode property specifies how alpha is interpreted. These
   * values are linear. If a baseColorTexture is specified, this value is multiplied with the texel
   * values.
   */
  const Color& getBaseColor() const { return _baseColor; }
  void setBaseColor(const Color& color) { _baseColor = color; }

  /*
   * The base color texture. This texture contains RGB(A) components in sRGB color space.
   * The first three components (RGB) specify the base color of the material. If the fourth
   * component (A) is present, it represents the alpha coverage of the material. Otherwise, an alpha
   * of 1.0 is assumed. The alphaMode property specifies how alpha is interpreted. The stored texels
   * must not be premultiplied.
   */
  const Shader_texture_input& getBaseColorTexture() const { return _textures[BaseColor]; }

  void setBaseColorTexture(const Shader_texture_input& baseColorTexture) {
    setTexture(baseColorTexture, _textures[BaseColor]);
  }

  /*
   * The metalness of the material. A value of 1.0 means the material is a metal.
   * A value of 0.0 means the material is a dielectric.
   * Values in between are for blending between metals and dielectrics such as dirty metallic
   * surfaces. This value is linear. If a metallicRoughnessTexture is specified, this value is
   * multiplied with the metallic texel values.
   */
  float getMetallicFactor() const { return _metallic; }

  void setMetallicFactor(float metallic) { _metallic = metallic; }

  /*
   * The roughness of the material. A value of 1.0 means the material is completely rough.
   * A value of 0.0 means the material is completely smooth. This value is linear.
   * If a metallicRoughnessTexture is specified, this value is multiplied with the roughness texel
   * values.
   */
  float getRoughnessFactor() const { return _roughness; }

  void setRoughnessFactor(float roughness) { _roughness = roughness; }

  const Color& getEmissiveFactor() const { return _emissive; }

  void setEmissiveFactor(const Color& emissive) { _emissive = emissive; }

  float getAlphaCutoff() const { return _alphaCutoff; }

  void setAlphaCutoff(float alphaCutoff) { _alphaCutoff = alphaCutoff; }

  /*
   * The metallic-roughness texture.
   * The metalness values are sampled from the B channel. The roughness values are sampled from the
   * G channel. These values are linear. If other channels are present (R or A), they are ignored
   * for metallic-roughness calculations.
   */
  const Shader_texture_input& getMetallicRoughnessTexture() const {
    return _textures[MetallicRoughness];
  }

  void setMetallicRoughnessTexture(const Shader_texture_input& metallicRoughnessTexture) {
    setTexture(metallicRoughnessTexture, _textures[MetallicRoughness]);
  }

  const Shader_texture_input& getNormalTexture() const { return _textures[Normal]; }

  void setNormalTexture(const Shader_texture_input& normalTexture) {
    setTexture(normalTexture, _textures[Normal]);
  }

  const Shader_texture_input& getOcclusionTexture() const { return _textures[Occlusion]; }

  void setOcclusionTexture(const Shader_texture_input& occlusionTexture) {
    setTexture(occlusionTexture, _textures[Occlusion]);
  }

  const Shader_texture_input& getEmissiveTexture() const { return _textures[Emissive]; }

  void setEmissiveTexture(const Shader_texture_input& emissiveTexture) {
    setTexture(emissiveTexture, _textures[Emissive]);
  }

  Material_light_mode lightMode() override { return Material_light_mode::Lit; }
  const std::string& materialType() const override;

  nlohmann::json serialize(bool compilerPropertiesOnly) const override;

  std::set<std::string> configFlags(
      const Renderer_features_enabled& rendererFeatures,
      Render_pass_blend_mode blendMode,
      const Mesh_vertex_layout& meshVertexLayout) const override;

  std::vector<Vertex_attribute_element> vertexInputs(
      const std::set<std::string>& flags) const override;
  Shader_resources shaderResources(
      const std::set<std::string>& flags,
      const Render_light_data& renderLightData) const override;
  Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override;
  Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override;

  static void decodeMorphTargetConfig(
      const std::set<std::string>& flags,
      uint8_t& targetCount,
      int8_t& positionPosition,
      int8_t& normalPosition,
      int8_t& tangentPosition);

  void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) override;

  Shader_constant_layout getShaderConstantLayout() const override;

  Shader_output_layout getShaderOutputLayout() const override
  {
    static constexpr std::array<Texture_format, 3> gbufferRtvFormats{
            {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT}
    };
    return Shader_output_layout{gbufferRtvFormats};
  }

 protected:
  void setTexture(const Shader_texture_input& src, Shader_texture_input& dest);

  void applyVertexLayoutShaderCompileSettings(Shader_compile_settings&) const;

  void updateVsConstantBufferValues(
      PBR_material_vs_constant_buffer& constants, const Update_constant_buffer_inputs& inputs) const;

  void updatePsConstantBufferValues(
      PBR_material_ps_constant_buffer& constants, const Update_constant_buffer_inputs& inputs) const;

  bool requiresTexCoord0() const {
    return std::any_of(
            _textures.begin(), _textures.end(), [](const Shader_texture_input& t) { return t.texture != nullptr; });
  }

  bool requiresTangents() const { return _textures[Normal].texture != nullptr; }

  static int getMorphPositionAttributeIndexOffset();
  static int getMorphNormalAttributeIndexOffset();
  int getMorphTangentAttributeIndexOffset() const;

 private:
  Color _baseColor;
  float _metallic;
  float _roughness;
  Color _emissive;
  float _alphaCutoff;

  std::array<Shader_texture_input, NumTextureTypes> _textures;
};
} // namespace oe
