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

struct Material_state_identifier {
  // hash of the states that the compiled shader relied on
  size_t depthStencilModeHash = 0;
  size_t materialHash = 0;
  size_t meshHash = 0;
  size_t rendererFeaturesHash = 0;
  bool wireframeEnabled = false;
};

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

  virtual bool getMaterialStateIdentifier(Material_context_handle handle, Material_state_identifier& stateIdentifier) = 0;

  void updateMaterialContext(
          Material_context_handle materialContextHandle, std::shared_ptr<const Material> material,
          const Mesh_vertex_layout& meshVertexLayout, const Mesh_gpu_data& meshGpuData,
          const Render_light_data* renderLightData, const Depth_stencil_config& depthStencilConfig,
          bool enablePixelShader, bool wireframe) override;

  // Sets the given material context for rendering
  void bind(Material_context_handle materialContextHandle) override;

  void unbind() override;

  void setRendererFeaturesEnabled(const Renderer_features_enabled& renderer_feature_enabled) override;
  const Renderer_features_enabled& rendererFeatureEnabled() const override;

 protected:
  void setShaderPath(const std::string& path);

  struct Material_compiler_inputs {
    Depth_stencil_config depthStencilConfig;
    std::vector<Vertex_attribute_element> vsInputs;
    std::set<std::string> flags;
    Mesh_index_type meshIndexType;
    bool enableOptimizations;

    // Used for debug identification
    std::string name;
  };
  virtual void loadMaterialToContext(
          Material_context_handle materialContextHandle, const Material& material,
          const Material_state_identifier& stateIdentifier,
          const Material_compiler_inputs& materialCompilerInputs) = 0;

  virtual void loadResourcesToContext(
          Material_context_handle materialContextHandle,
          const Material::Shader_resources& shader_resources,
          const std::vector<Vertex_attribute_element>& vsInputs) = 0;

  struct Pipeline_state_inputs {
    // Options that don't need a recompile, but will need to modify pipeline state
    bool wireframe;
  };
  virtual void loadPipelineStateToContext(Material_context_handle materialContextHandle,
                                          const Pipeline_state_inputs& inputs) = 0;

  virtual void bindMaterialContextToDevice(Material_context_handle materialContextHandle) = 0;

  virtual void setDataReady(Material_context_handle materialContextHandle, bool ready) = 0;

  /**
   * Helper that logs dumps a JSON representation of the given settings object to the debug log. Does nothing if the
   * debug log level is not enabled.
   */
  void debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings) const;

 private:
  virtual void queueTextureLoad(const Texture& texture) = 0;

  std::string _shaderPath = "data/shaders";

  Renderer_features_enabled _rendererFeatures;
  size_t _rendererFeaturesHash = 0;

  bool _boundMaterial = false;

  IAsset_manager& _assetManager;
};
}// namespace oe
