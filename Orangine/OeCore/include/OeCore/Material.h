#pragma once

#include "Mesh_data_component.h"
#include "Renderer_types.h"

#include "Renderer_data.h"

#include <set>
#include <string>
#include <gsl/span>

#include <json.hpp>
#include <vectormath.hpp>

namespace oe {
class Mesh_vertex_layout;
class Render_light_data;
class Texture;

class Material {
 public:
  struct Shader_compile_settings {
    std::string filename;
    std::string entryPoint;
    std::map<std::string, std::string> defines;
    std::set<std::string> includes;
    std::vector<Vertex_attribute_semantic> morphAttributes;
  };

  struct Shader_resources {
    // Ordered list of textures that will exposed to the shader as shaderResourceViews
    std::vector<std::shared_ptr<Texture>> textures;

    // Ordered list of samplers that are given to the shader.
    std::vector<Sampler_descriptor> samplerDescriptors;
  };

  Material(
      uint8_t materialTypeIndex,
      Material_alpha_mode alphaMode,
      Material_face_cull_mode faceCullMode);

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
  void setAlphaMode(Material_alpha_mode mode) {
    _alphaMode = mode;
    markRequiresRecompile();
  }

  Material_face_cull_mode faceCullMode() const { return _faceCullMode; }
  void setFaceCullMode(Material_face_cull_mode mode) {
    _faceCullMode = mode;
    markRequiresRecompile();
  }

  virtual Material_light_mode lightMode() { return Material_light_mode::Unlit; }
  virtual const std::string& materialType() const = 0;

  /*
   * Helpers that are called by the Material_manager
   */

  // Creates a d3d constant buffer
  virtual void updateVsConstantBuffer(
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      const Renderer_animation_data& rendererAnimationData,
      uint8_t* buffer,
      size_t bufferSize) const {}

  virtual void updatePsConstantBuffer(
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      uint8_t* buffer,
      size_t bufferSize) const {};

  virtual std::vector<Vertex_attribute_element> vertexInputs(
      const std::set<std::string>& flags) const = 0;

  // Used at material compile time - determines flags that are later passed in to the shaderSettings
  // methods.
  virtual std::set<std::string> configFlags(
      const Renderer_features_enabled& rendererFeatures,
      Render_pass_blend_mode blendMode,
      const Mesh_vertex_layout& meshBindContext) const;
  // Used at compile time
  virtual Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const;
  // Used at compile time
  virtual Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const;

  // Used at bind time - specifies which textures and samplers are required.
  virtual Shader_resources shaderResources(
      const std::set<std::string>& flags,
      const Render_light_data& renderLightData) const;

  virtual Shader_constant_layout getShaderConstantLayout() const = 0;

  /*
   * Per Frame
   */
  virtual void setContextSamplers(const Render_light_data& renderLightData) const {}
  virtual void unsetContextSamplers() const {}

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

 protected:
  void markRequiresRecompile() { _requiresRecompile = true; }

  static nlohmann::json serializeTexture(
      bool compilerPropertiesOnly,
      const std::shared_ptr<Texture>& texture);

 private:
  bool _requiresRecompile;

  const uint8_t _materialTypeIndex;
  Material_alpha_mode _alphaMode;
  Material_face_cull_mode _faceCullMode;
  size_t _propertiesHash = 0;
};

} // namespace oe
