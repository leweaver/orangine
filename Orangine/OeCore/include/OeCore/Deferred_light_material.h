#pragma once

#include "OeCore/Material.h"
#include "OeCore/Material_base.h"

namespace oe {
class Render_target_texture;

// This constant buffer contains data that is shared across each light that is rendered, such as the
// camera data. Individual light parameters are stored in the constant buffer managed by the
// Render_light_data_impl class.
struct Deferred_light_material_constant_buffer : Pixel_constant_buffer_base {
  SSE::Matrix4 viewMatrixInv;
  SSE::Matrix4 projMatrixInv;
  SSE::Vector4 eyePosition;
  bool emittedEnabled = false;
};

class Deferred_light_material
    : public Material_base<Vertex_constant_buffer_empty, Deferred_light_material_constant_buffer> {
  using Base_type =
      Material_base<Vertex_constant_buffer_empty, Deferred_light_material_constant_buffer>;

 public:
  static constexpr uint32_t max_lights = 8;

  Deferred_light_material();

  const std::shared_ptr<Texture>& color0Texture() const { return _color0Texture; }
  void setColor0Texture(const std::shared_ptr<Texture>& texture) { _color0Texture = texture; }
  const std::shared_ptr<Texture>& color1Texture() const { return _color1Texture; }
  void setColor1Texture(const std::shared_ptr<Texture>& texture) { _color1Texture = texture; }
  const std::shared_ptr<Texture>& color2Texture() const { return _color2Texture; }
  void setColor2Texture(const std::shared_ptr<Texture>& texture) { _color2Texture = texture; }
  const std::shared_ptr<Texture>& depthTexture() const { return _depthTexture; }
  void setDepthTexture(const std::shared_ptr<Texture>& texture) { _depthTexture = texture; }
  const std::shared_ptr<Texture>& shadowMapDepthTexture() const { return _shadowMapDepthTexture; }
  void setShadowMapDepthTexture(const std::shared_ptr<Texture>& texture) {
    _shadowMapDepthTexture = texture;
  }
  const std::shared_ptr<Texture>& shadowMapStencilTexture() const {
    return _shadowMapStencilTexture;
  }
  void setShadowMapStencilTexture(const std::shared_ptr<Texture>& texture) {
    _shadowMapStencilTexture = texture;
  }
  bool iblEnabled() const { return _iblEnabled; }
  void setIblEnabled(bool iblEnabled) { _iblEnabled = iblEnabled; }
  bool shadowArrayEnabled() const { return _shadowArrayEnabled; }
  void setShadowArrayEnabled(bool shadowArrayEnabled) { _shadowArrayEnabled = shadowArrayEnabled; }

  void setupEmitted(bool enabled);

  Material_light_mode lightMode() override { return Material_light_mode::Lit; }
  const std::string& materialType() const override;

  nlohmann::json serialize(bool compilerPropertiesOnly) const;

  std::set<std::string> configFlags(
      const Renderer_features_enabled& rendererFeatures,
      Render_pass_blend_mode blendMode,
      const Mesh_vertex_layout& meshBindContext) const override;
  Shader_resources shaderResources(
      const std::set<std::string>& flags,
      const Render_light_data& renderLightData) const override;

 protected:
  Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override;

  void updatePsConstantBufferValues(
      Deferred_light_material_constant_buffer& constants,
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix) const override;

 private:
  std::shared_ptr<Texture> _color0Texture;
  std::shared_ptr<Texture> _color1Texture;
  std::shared_ptr<Texture> _color2Texture;
  std::shared_ptr<Texture> _depthTexture;
  std::shared_ptr<Texture> _shadowMapDepthTexture;
  std::shared_ptr<Texture> _shadowMapStencilTexture;

  /*
          Microsoft::WRL::ComPtr<ID3D11SamplerState> _color0SamplerState;
          Microsoft::WRL::ComPtr<ID3D11SamplerState> _color1SamplerState;
          Microsoft::WRL::ComPtr<ID3D11SamplerState> _color2SamplerState;
          Microsoft::WRL::ComPtr<ID3D11SamplerState> _depthSamplerState;
          Microsoft::WRL::ComPtr<ID3D11SamplerState> _shadowMapSamplerState;
  */

  int32_t _shadowMapCount = 0;
  bool _emittedEnabled = false;
  bool _iblEnabled = true;
  bool _shadowArrayEnabled = true;
};
} // namespace oe
