#pragma once

#include "Manager_base.h"
#include "Render_pass.h"
#include "Renderer_data.h"
#include "Renderer_types.h"
#include "Render_light_data.h"

namespace oe {
struct Mesh_gpu_data;
class Material_context;
class Render_light_data;
class Scene;
class Material;
class Mesh_vertex_layout;
struct Renderer_animation_data;

class IMaterial_manager {
 public:
  // Gets the path that contains hlsl files. Does not end in a trailing slash.
  virtual const std::string& shaderPath() const = 0;

  // Creates a material context that may be destroyed when the device is reset or shutdown.
  virtual Material_context_handle createMaterialContext(const std::string& name) = 0;

  // Compiles (if needed) pixel and vertex shaders, uploads samplers & textures.
  virtual void updateMaterialContext(
          Material_context_handle materialContext,
          std::shared_ptr<const Material> material,
          const Mesh_vertex_layout& meshVertexLayout,
          const Mesh_gpu_data& meshGpuData,
          const Render_light_data* renderLightData,
          const Depth_stencil_config& stencilConfig,
          Render_pass_target_layout targetLayout,
          bool enablePixelShader,
          bool wireframe) = 0;

  virtual bool isMaterialContextDataReady(Material_context_handle materialContext) const = 0;

  // Sets the given material context for rendering
  virtual void bind(Material_context_handle materialContext) = 0;

  // Unbinds the current material. Must be called when rendering of an object is complete.
  virtual void unbind() = 0;

  virtual void setRendererFeaturesEnabled(
      const Renderer_features_enabled& renderer_feature_enabled) = 0;
  virtual const Renderer_features_enabled& rendererFeatureEnabled() const = 0;

  virtual void updateLightBuffers() = 0;
};
} // namespace oe
