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

class Material_manager : public IMaterial_manager,
                         public Manager_base,
                         public Manager_tickable,
                         public Manager_deviceDependent {
 public:
  explicit Material_manager(IAsset_manager& assetManager);
  ~Material_manager() = default;

  Material_manager(const Material_manager& other) = delete;
  Material_manager(Material_manager&& other) = delete;
  void operator=(const Material_manager& other) = delete;
  void operator=(Material_manager&& other) = delete;

  // Manager_base implementation
  void initialize() override;
  void shutdown() override {}

  // Manager_tickable implementation
  void tick() override;

  // IMaterial_manager implementation
  const std::string& shaderPath() const;

  void updateMaterialContext(
          Material_context& materialContext, std::shared_ptr<const Material> material,
          const Mesh_vertex_layout& meshVertexLayout, const Mesh_gpu_data& meshGpuData,
          const Render_light_data* renderLightData, const Depth_stencil_config& depthStencilConfig,
          bool enablePixelShader, bool wireframe) override;

  // Sets the given material context for rendering
  void bind(Material_context& materialContext) override;

  void unbind() override;

  void setRendererFeaturesEnabled(const Renderer_features_enabled& renderer_feature_enabled) override;
  const Renderer_features_enabled& rendererFeatureEnabled() const override;

 protected:
  void setShaderPath(const std::string& path);

  virtual void loadMaterialToContext(const Material& material, Material_context& materialContext, bool enableOptimizations) = 0;

  virtual void loadResourcesToContext(
          const Material::Shader_resources& shader_resources,
          const std::vector<Vertex_attribute_element>& vsInputs,
          Material_context& materialContext) = 0;

  virtual void loadPipelineStateToContext(Material_context& materialContext) = 0;

  virtual void bindMaterialContextToDevice(const Material_context& materialContext) = 0;

  /**
   * Helper that logs dumps a JSON representation of the given settings object to the debug log. Does nothing if the
   * debug log level is not enabled.
   */
  void debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings) const;

 private:
  std::string _shaderPath = "data/shaders";

  Renderer_features_enabled _rendererFeatures;
  size_t _rendererFeaturesHash = 0;

  bool _boundMaterial = false;

  IAsset_manager& _assetManager;
};
}// namespace oe
