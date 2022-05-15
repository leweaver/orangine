#pragma once

#include "D3D12_device_resources.h"
#include "D3D12_renderer_types.h"
#include "Gpu_buffer.h"

#include <OeCore/Entity_render_manager.h>
#include <OeCore/Renderer_data.h>

namespace oe::pipeline_d3d12 {
class D3D12_entity_render_manager : public internal::Entity_render_manager {
 public:
  D3D12_entity_render_manager(
          ITexture_manager& textureManager, IMaterial_manager& materialManager, ILighting_manager& lightingManager,
          IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources)
      : Entity_render_manager(textureManager, materialManager, lightingManager, primitiveMeshDataFactory)
      , _deviceResources(deviceResources)
  {}

  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

 protected:
  void drawRendererData(
          const Camera_data& cameraData, const Matrix4& worldTransform, Mesh_gpu_data& rendererData,
          const Depth_stencil_config& depthStencilConfig, const Render_light_data& renderLightData,
          std::shared_ptr<Material> ptr, const Mesh_vertex_layout& meshVertexLayout, Material_context& materialContext,
          Renderer_animation_data& rendererAnimationData, bool wireframe) override;

  std::shared_ptr<Mesh_gpu_data> createRendererData(
          std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& vertexAttributes,
          const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) override;

  std::shared_ptr<Gpu_buffer_reference>
  getOrCreateUsage(const std::string_view& name, const std::shared_ptr<Mesh_buffer>& meshData, size_t vertexSize);

 private:
  D3D12_device_resources& _deviceResources;

  struct {
    std::vector<std::shared_ptr<Mesh_gpu_data>> createdRendererData;
    ID3D12Device6* device = nullptr;
    std::unordered_map<std::shared_ptr<Mesh_buffer>, std::shared_ptr<Gpu_buffer_reference>> meshBufferToGpuBuffers;
  } _deviceDependent;
};
}// namespace oe::pipeline_d3d12