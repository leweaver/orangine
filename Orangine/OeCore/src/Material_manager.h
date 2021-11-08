#pragma once

#include "OeCore/IMaterial_manager.h"
#include "OeCore/Material.h"
#include "OeCore/Renderer_data.h"
#include "OeCore/Renderer_types.h"
#include <OeCore/IAsset_manager.h>

namespace oe {
class Texture;
class Scene;
struct Renderer_animation_data;

class Material_manager : public IMaterial_manager {
 public:
  Material_manager(Scene& scene, IAsset_manager& assetManager);
  ~Material_manager() = default;

  Material_manager(const Material_manager& other) = delete;
  Material_manager(Material_manager&& other) = delete;
  void operator=(const Material_manager& other) = delete;
  void operator=(Material_manager&& other) = delete;

  // Manager_base implementation
  void initialize() override;
  void shutdown() override {}
  const std::string& name() const override;

  // Manager_tickable implementation
  void tick() override;

  // IMaterial_manager implementation
  const std::wstring& shaderPath() const;

  void bind(
      Material_context& materialContext,
      std::shared_ptr<const Material> material,
      const Mesh_vertex_layout& meshVertexLayout,
      const Render_light_data* renderLightData,
      Render_pass_blend_mode blendMode,
      bool enablePixelShader) override;

  void unbind() override;

  void setRendererFeaturesEnabled(
      const Renderer_features_enabled& renderer_feature_enabled) override;
  const Renderer_features_enabled& rendererFeatureEnabled() const override;

 protected:
  void setShaderPath(const std::wstring& path);

  const Material* getBoundMaterial() const { return _boundMaterial.get(); }

  virtual void createVertexShader(
      bool enableOptimizations,
      const Material& material,
      Material_context& materialContext) const = 0;
  virtual void createPixelShader(
      bool enableOptimizations,
      const Material& material,
      Material_context& materialContext) const = 0;

  virtual void createMaterialConstants(const Material& material) = 0;

  virtual void loadShaderResourcesToContext(
      const Material::Shader_resources& shader_resources,
      Material_context& material_context) = 0;

  virtual void bindLightDataToDevice(const Render_light_data* renderLightData) = 0;
  virtual void bindMaterialContextToDevice(
      const Material_context& materialContext,
      bool enablePixelShader) = 0;

 private:
  static std::string _name;

  std::wstring _shaderPath = L"data/shaders";

  Renderer_features_enabled _rendererFeatures;
  size_t _rendererFeaturesHash = 0;

  std::shared_ptr<const Material> _boundMaterial;
  Render_pass_blend_mode _boundBlendMode;
  IAsset_manager& _assetManager;
};
} // namespace oe
