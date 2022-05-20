#pragma once
#include "Material.h"

namespace oe {
__declspec(align(16))
struct Skybox_material_vs_constant_buffer {
  SSE::Matrix4 worldViewProjection;
};

class Skybox_material : public Material {
 public:
  inline static constexpr uint32_t kCbRegisterVsMain = 0;

  Skybox_material();

  Skybox_material(const Skybox_material& other) = delete;
  Skybox_material(Skybox_material&& other) = delete;
  void operator=(const Skybox_material& other) = delete;
  void operator=(Skybox_material&& other) = delete;

  virtual ~Skybox_material() = default;

  std::shared_ptr<Texture> getCubeMapTexture() const { return _cubeMapTexture; }
  void setCubeMapTexture(std::shared_ptr<Texture> cubeMapTexture)
  {
    OE_CHECK(cubeMapTexture == nullptr || cubeMapTexture->isCubeMap());
    _cubeMapTexture = cubeMapTexture;
  }

  const std::string& materialType() const override;

  nlohmann::json serialize(bool compilerPropertiesOnly) const override;
  Shader_resources
  shaderResources(const std::set<std::string>& flags, const Render_light_data& renderLightData) const override;

  void updatePerDrawConstantBuffer(
          gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
          const Update_constant_buffer_inputs& inputs) override;

  Shader_constant_layout getShaderConstantLayout() const override;

 private:
  std::shared_ptr<Texture> _cubeMapTexture;
};
}// namespace oe
