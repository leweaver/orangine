#pragma once

#include "OeCore/Collision.h"
#include "OeCore/Environment_volume.h"
#include "OeCore/IEntity_render_manager.h"
#include "OeCore/Light_provider.h"
#include "OeCore/Mesh_data_component.h"
#include "OeCore/Render_light_data.h"
#include "OeCore/Render_pass.h"
#include "OeCore/Renderable.h"

#include <memory>

namespace oe {
class Material_context;
class ITexture_manager;
class IMaterial_manager;
class ILighting_manager;
}// namespace oe

namespace oe::internal {
class Entity_render_manager : public IEntity_render_manager, public Manager_base, public Manager_deviceDependent {
 public:
  Entity_render_manager(
          ITexture_manager& textureManager, IMaterial_manager& materialManager, ILighting_manager& lightingManager,
          IPrimitive_mesh_data_factory& primitiveMeshDataFactory);

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_deviceDependent must be implemented by subclass

  // IEntity_render_manager implementation
  void renderRenderable(
          Renderable& renderable, const SSE::Matrix4& worldMatrix, float radius, const Camera_data& cameraData,
          const Light_provider::Callback_type& lightDataProvider, const Depth_stencil_config& depthStencilConfig,
          bool wireFrame) override;

  void renderEntity(
          Renderable_component& renderable, const Camera_data& cameraData,
          const Light_provider::Callback_type& lightDataProvider, const Depth_stencil_config& blendMode) override;

  // Be warned - YOU are responsible for cleaning up this Renderable's D3D data on a device reset.
  Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) override;

  void clearRenderStats() override;

 protected:
  void createMissingVertexAttributes(
          std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& requiredAttributes,
          const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) const;

  virtual void drawRendererData(
          const Camera_data& cameraData, const SSE::Matrix4& worldTransform, Mesh_gpu_data& rendererData,
          const Depth_stencil_config& depthStencilConfig, const Render_light_data& renderLightData,
          std::shared_ptr<Material>, const Mesh_vertex_layout& meshVertexLayout, Material_context& materialContext,
          Renderer_animation_data& rendererAnimationData, bool wireframe) = 0;

  /**
   * \brief Creates a new Renderer_data instance from the given mesh data, in a format
   *        that satisfies the given vertex attributes (normally generated from a material)
   * \param meshData
   * \param vertexAttributes Semantics that the material requested for vertices.
   *        Normally you should get this from Material::vertexInputs
   * \param vertexMorphAttributes Semantics that the material requested for morph streams.
   *        Normally you should get this from Shader_compile_settings::morphAttributes
   * \return new Renderer_data instance.
   */
  virtual std::shared_ptr<Mesh_gpu_data> createRendererData(
          std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& vertexAttributes,
          const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) = 0;

  Mesh_vertex_buffer_accessor* findAccessorForSemantic(
          std::shared_ptr<Mesh_data>& meshData, const gsl::span<const Vertex_attribute_semantic>& attributes,
          const Vertex_attribute_semantic& vertexAttrSemantic) const;

  struct Render_stats {
    int opaqueEntityCount = 0;
    int opaqueLightCount = 0;
    int alphaEntityCount = 0;
  };

  // Rendering
  Render_stats _renderStats = {};
  Renderer_animation_data _rendererAnimationData = {};
  std::vector<Entity*> _renderLights = {};

  ITexture_manager& _textureManager;
  IMaterial_manager& _materialManager;
  ILighting_manager& _lightingManager;
  IPrimitive_mesh_data_factory& _primitiveMeshDataFactory;

  static std::string _name;
};
}// namespace oe::internal
