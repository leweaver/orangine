#pragma once

#include "Manager_base.h"
#include "Render_pass.h"
#include "Renderer_data.h"
#include "Renderer_types.h"

namespace oe {
struct Renderer_data;
class Material_context;
class Render_light_data;
class Scene;
class Material;
class Mesh_vertex_layout;
struct Renderer_animation_data;

class IMaterial_manager
    : public Manager_base
    , public Manager_tickable
    , public Manager_deviceDependent {
 public:
  explicit IMaterial_manager(Scene& scene) : Manager_base(scene) {}

  // Gets the path that contains hlsl files. Does not end in a trailing slash.
  virtual const std::wstring& shaderPath() const = 0;

  // Creates a material context that may be destroyed when the device is reset or shutdown.
  virtual std::weak_ptr<Material_context> createMaterialContext() = 0;

  // Sets the Material that is used for all subsequent calls to render.
  // Compiles (if needed), binds pixel and vertex shaders, and textures.
  virtual void bind(
      Material_context& materialContext,
      std::shared_ptr<const Material> material,
      const Mesh_vertex_layout& meshVertexLayout,
      const Render_light_data* renderLightData,
      Render_pass_blend_mode blendMode,
      bool enablePixelShader) = 0;

  // Uploads shader constants, then renders
  virtual void render(
      const Renderer_data& rendererData,
      const SSE::Matrix4& worldMatrix,
      const Renderer_animation_data& rendererAnimationState,
      const Camera_data& camera) = 0;

  // Unbinds the current material. Must be called when rendering of an object is complete.
  virtual void unbind() = 0;

  virtual void setRendererFeaturesEnabled(
      const Renderer_features_enabled& renderer_feature_enabled) = 0;
  virtual const Renderer_features_enabled& rendererFeatureEnabled() const = 0;
};
} // namespace oe
