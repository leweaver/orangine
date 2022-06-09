#pragma once

#include "Mesh_data_component.h"
#include "Renderer_types.h"

#include "Renderer_data.h"

#include <gsl/span>
#include <set>
#include <string>

#include <json.hpp>
#include <vectormath.hpp>

namespace oe {
class Mesh_vertex_layout;
class Render_light_data;
class Texture;

class Material {
 public:
  inline static constexpr Shader_external_constant_buffer_handle kExternalUsageLightingHandle = 1;
  inline static constexpr Shader_external_constant_buffer_handle kExternalUsageSkeletonHandle = 2;

  struct Shader_compile_settings {
    std::string filename;
    std::string entryPoint;
    std::map<std::string, std::string> defines;
    std::set<std::string> includes;
    std::vector<Vertex_attribute_semantic> morphAttributes;
  };

  struct Shader_texture_resource {
    std::shared_ptr<Texture> texture;
    Shader_texture_resource_format format = Shader_texture_resource_format::Default;
  };

  struct Shader_resources {
    // Ordered list of textures that will be exposed to the shader as shaderResourceViews
    std::vector<Shader_texture_resource> textures;

    // Ordered list of samplers that are given to the shader. The size of this array doesn't have to match the number of
    // textures - it's up to the shader/material.
    std::vector<Sampler_descriptor> samplerDescriptors;
  };

  struct Shader_texture_input {
    std::shared_ptr<Texture> texture;
    Sampler_descriptor samplerDescriptor;
  };

  Material(
          uint8_t materialTypeIndex, Material_alpha_mode alphaMode = Material_alpha_mode::Opaque,
          Material_face_cull_mode faceCullMode = Material_face_cull_mode::Back_face);

  Material(const Material& other) = delete;
  Material(Material&& other) = delete;
  void operator=(const Material& other) = delete;
  void operator=(Material&& other) = delete;

  virtual ~Material() = default;

  /*
   * This index is unique for a given material type. For example, ALL instances of a PBRMaterial
   * will return the same index.
   *
   * This is used by Material_manager to manage resources. As a practical example, this is used
   * to differentiate constant buffers.
   *
   * This value is not guaranteed to be the same between re-runs - it should never be persisted
   * to disk.
   */
  uint8_t materialTypeIndex() const { return _materialTypeIndex; }

  Material_alpha_mode getAlphaMode() const { return _alphaMode; }
  void setAlphaMode(Material_alpha_mode mode)
  {
    _alphaMode = mode;
    markRequiresRecompile();
  }

  Material_face_cull_mode faceCullMode() const { return _faceCullMode; }
  void setFaceCullMode(Material_face_cull_mode mode)
  {
    _faceCullMode = mode;
    markRequiresRecompile();
  }

  virtual Material_light_mode lightMode() { return Material_light_mode::Unlit; }
  virtual const std::string& materialType() const = 0;

  struct Update_constant_buffer_inputs {
    const Matrix4& worldTransform;
    const SSE::Matrix4& viewMatrix;
    const SSE::Matrix4& projectionMatrix;
    const oe::Render_light_data& renderLightData;
    const oe::Renderer_animation_data& rendererAnimationData;
  };
  /**
   * Updates the given buffer with constant values memory matching the given descriptor
   * @param cpuBuffer Target memory
   * @param bufferDesc Which constant buffer format to populate the buffer with
   * @param inputs Reference that is used to populate constant buffer values
   */
  virtual void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) = 0;

  virtual std::vector<Vertex_attribute_element> vertexInputs(const std::set<std::string>& flags) const {
    std::vector<Vertex_attribute_element> vsInputs{
            {{Vertex_attribute::Position, 0}, Element_type::Vector3, Element_component::Float}};
    return vsInputs;
  }

  // Used at material compile time - determines flags that are later passed in to the shaderSettings
  // methods.
  virtual std::set<std::string> configFlags(
          const Renderer_features_enabled& rendererFeatures,
          Render_pass_target_layout targetLayout,
          const Mesh_vertex_layout& meshBindContext) const;
  // Used at compile time
  virtual Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const;
  // Used at compile time
  virtual Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const;

  // Used at bind time - specifies which textures and samplers are required.
  virtual Shader_resources
  shaderResources(const std::set<std::string>& flags, const Render_light_data& renderLightData) const;

  virtual Shader_constant_layout getShaderConstantLayout() const = 0;

  /**
   * @return The render target format of the pixel shader
   */
  virtual Shader_output_layout getShaderOutputLayout() const
  {
    // default layout consisting of a single RGBA render target.
    static constexpr std::array<Texture_format, 1> baseRtvFormats{{DXGI_FORMAT_B8G8R8A8_UNORM}};
    return Shader_output_layout{baseRtvFormats};
  }

  // Returns a std::hash of all properties that affect the result of shader compilation.
  // If the material is marked as requiring a recompile, this method will compute the hash prior to
  // returning it. This is cached on the material instance and will only computer the hash if one of
  // the properties changes.
  size_t calculateCompilerPropertiesHash();

  // Same as calculateCompilerPropertiesHash; but if the hash isn't up to date, will throw an
  // exception.
  size_t ensureCompilerPropertiesHash() const;

  // Serialize properties to a map
  virtual nlohmann::json serialize(bool compilerPropertiesOnly) const;

  virtual const gsl::span<const Render_pass_target_layout> getAllowedTargetFlags() const;

 protected:
  void markRequiresRecompile() { _requiresRecompile = true; }

  static nlohmann::json serializeTexture(bool compilerPropertiesOnly, const Shader_texture_input& textureInput) {
    return serializeTexture(compilerPropertiesOnly, textureInput.texture);
  }

  static nlohmann::json serializeTexture(bool compilerPropertiesOnly, const std::shared_ptr<Texture>& texture);

 private:
  bool _requiresRecompile;

  const uint8_t _materialTypeIndex;
  Material_alpha_mode _alphaMode;
  Material_face_cull_mode _faceCullMode;
  size_t _propertiesHash = 0;
};

}// namespace oe
