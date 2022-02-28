#pragma once

#include "D3D12_device_resources.h"
#include "Gpu_buffer.h"

#include <OeCore/Entity_render_manager.h>
#include <OeCore/Renderer_data.h>

namespace oe {
struct Renderer_data {
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

  unsigned int vertexCount;
  std::unordered_map<Vertex_attribute_semantic, std::unique_ptr<pipeline_d3d12::Gpu_buffer>> vertexBuffers;

  std::unique_ptr<pipeline_d3d12::Gpu_buffer> indexBuffer;
  DXGI_FORMAT indexFormat;
  unsigned int indexCount;

  bool failedRendering;
};
}

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
          const Camera_data& cameraData, const Matrix4& worldTransform, Renderer_data& rendererData,
          Render_pass_blend_mode blendMode, const Render_light_data& renderLightData, std::shared_ptr<Material> ptr,
          const Mesh_vertex_layout& meshVertexLayout, Material_context& materialContext,
          Renderer_animation_data& rendererAnimationData, bool wireframe) override;

  void loadRendererDataToDeviceContext(const Renderer_data& rendererData, const Material_context& context) override;

  std::shared_ptr<Renderer_data> createRendererData(
          std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& vertexAttributes,
          const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) override;

 private:
  std::unique_ptr<Gpu_buffer> createGpuReadBuffer(const Mesh_buffer& meshBuffer, size_t strideInBytes);

  D3D12_device_resources& _deviceResources;

  struct {
    // TODO: Perhaps one per step?
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;

    std::vector<std::shared_ptr<Renderer_data>> createdRendererData;
    ID3D12Device6* device;
  } _deviceDependent;
};
}// namespace oe::pipeline_d3d12