#pragma once

#include "D3D12_device_resources.h"
#include "Descriptor_heap_pool.h"
#include "Gpu_buffer.h"

#include <OeCore/Material_context.h>
#include <OeCore/Material_manager.h>


namespace oe::pipeline_d3d12 {

struct Material_gpu_constant_buffer_table {
  std::vector<Gpu_buffer*> buffers;
  Descriptor_heap_pool::Descriptor_range descriptorRange;
};

using VisibilityGpuConstantBufferTablePair = std::pair<D3D12_SHADER_VISIBILITY, Material_gpu_constant_buffer_table>;
using VisibilityGpuConstantBufferTablePairs = std::vector<VisibilityGpuConstantBufferTablePair>;
struct Material_gpu_constant_buffers {
  // Owns the buffer data in the following tables. Not really intended for anything other than a container for cleanup
  // later.
  std::vector<std::unique_ptr<Gpu_buffer>> gpuBuffers;

  VisibilityGpuConstantBufferTablePairs tables;
};

class Material_context_impl : public Material_context {
 public:
  ~Material_context_impl()
  {
    releaseResources();
  }
  void releaseResources() override
  {
    releasePipelineState();
  };

  void releasePipelineState()
  {
    vertexShader.Reset();
    pixelShader.Reset();
    pipelineState.Reset();
  }

  Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
  Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
  Descriptor_heap_pool::Descriptor_range samplerDescriptorTable;
  Descriptor_heap_pool::Descriptor_range srvDescriptorTable;

  std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
  D3D12_INDEX_BUFFER_VIEW indexBufferView;

  Material_gpu_constant_buffers perDrawConstantBuffers;
  Material_gpu_constant_buffers* perMaterialConstantBuffers = nullptr;
  bool perDrawConstantBuffersInitialized = false;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

class D3D12_material_manager : public Material_manager {
 public:
  static constexpr uint32_t kDescriptorPoolSize = 100;
  D3D12_material_manager(
          IAsset_manager& assetManager, ITexture_manager& textureManager, ILighting_manager& lightingManager,
          IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources);
  ~D3D12_material_manager() override = default;

  D3D12_material_manager(const D3D12_material_manager& other) = delete;
  D3D12_material_manager(D3D12_material_manager&& other) = delete;
  void operator=(const D3D12_material_manager& other) = delete;
  void operator=(D3D12_material_manager&& other) = delete;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // IMaterial_manager implementation
  std::weak_ptr<Material_context> createMaterialContext() override;

  void createVertexShader(
          bool enableOptimizations, const Material& material, Material_context& materialContext) const override;

  void createPixelShader(
          bool enableOptimizations, const Material& material, Material_context& materialContext) const override;

  void createConstantBuffers(const Material& material, Material_context& materialContext) override;

  void loadShaderResourcesToContext(
          const Material::Shader_resources& shaderResources, Material_context& materialContext) override;

  void loadMeshGpuDataToContext(
          const Mesh_gpu_data& gpuData, const std::vector<Vertex_attribute_element>& vsInputs,
          Material_context& materialContext) override;

  void bindMaterialContextToDevice(const Material_context& materialContext, bool enablePixelShader) override;
  void
  render(const Material_context& materialContext, const SSE::Matrix4& worldMatrix,
         const Renderer_animation_data& rendererAnimationData, const Camera_data& camera) override;
  void unbind() override {}

  void updateLightBuffers() override {}

 private:
  inline Material_context_impl& getMaterialContextImpl(Material_context& context)
  {
    return static_cast<Material_context_impl&>(context);
  }

  inline const Material_context_impl& getMaterialContextImpl(const Material_context& context) const
  {
    return static_cast<const Material_context_impl&>(context);
  }

  static D3D12_PRIMITIVE_TOPOLOGY_TYPE getMeshDataTopology(Mesh_index_type meshIndexType);

  void compileShader(const Material::Shader_compile_settings& compileSettings, ID3DBlob** result) const;

  // Adds mappings of shader visibility to buffer table. Any buffer visibilities on the material will be split up into
  // duplicate entries for both PIXEL and VERTEX D3D12 visibilities.
  void createConstantBuffersForUsage(
          const Material& material, const Shader_constant_buffer_usage& usage,
          Material_gpu_constant_buffers& constantBuffers);

  IAsset_manager& _assetManager;
  ITexture_manager& _textureManager;
  ILighting_manager& _lightingManager;
  IPrimitive_mesh_data_factory& _primitiveMeshDataFactory;
  D3D12_device_resources& _deviceResources;

  struct Device_dependent_fields {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;

    std::vector<std::shared_ptr<Material_context_impl>> materialContexts;
    ID3D12Device6* device = nullptr;

    Descriptor_heap_pool srvHeap = {};
    Descriptor_heap_pool samplerHeap = {};

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    std::unordered_map<const Material*, std::unique_ptr<Material_gpu_constant_buffers>> perMaterialConstantBuffers;
  } _deviceDependent;
};
}// namespace oe::pipeline_d3d12