#pragma once

#include <OeCore/Material.h>

namespace oe {
class Deferred_light_material : public Material {
 public:
  inline static constexpr uint32_t kMaxLights = 8;
  inline static constexpr uint32_t kCbRegisterPsMain = 0;

  Deferred_light_material();

  const std::shared_ptr<Texture>& color0Texture() const { return _color0Texture; }
  void setColor0Texture(const std::shared_ptr<Texture>& texture) { _color0Texture = texture; }
  const std::shared_ptr<Texture>& color1Texture() const { return _color1Texture; }
  void setColor1Texture(const std::shared_ptr<Texture>& texture) { _color1Texture = texture; }
  const std::shared_ptr<Texture>& color2Texture() const { return _color2Texture; }
  void setColor2Texture(const std::shared_ptr<Texture>& texture) { _color2Texture = texture; }
  const std::shared_ptr<Texture>& depthTexture() const { return _depthTexture; }
  void setDepthTexture(const std::shared_ptr<Texture>& texture) { _depthTexture = texture; }
  const std::shared_ptr<Texture>& shadowMapArrayTexture() const { return _shadowMapArrayTexture; }
  void setShadowMapArrayTexture(const std::shared_ptr<Texture>& texture) {
    _shadowMapArrayTexture = texture;
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

  void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) override;

  Shader_constant_layout getShaderConstantLayout() const override;

 protected:
  Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override;

 private:
  std::shared_ptr<Texture> _color0Texture;
  std::shared_ptr<Texture> _color1Texture;
  std::shared_ptr<Texture> _color2Texture;
  std::shared_ptr<Texture> _depthTexture;
  std::shared_ptr<Texture> _shadowMapArrayTexture;

  int32_t _shadowMapCount = 0;
  bool _emittedEnabled = false;
  bool _iblEnabled = true;
  bool _shadowArrayEnabled = true;
};
} // namespace oe
