#pragma once

#include "Constant_buffer_pool.h"
#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"
#include "Descriptor_heap_pool.h"
#include "Gpu_buffer.h"

#include <OeCore/Material_manager.h>

#include <vector>

namespace oe::pipeline_d3d12 {

struct Material_gpu_constant_buffer_table {
  std::vector<Gpu_buffer_reference*> buffers;
  Descriptor_range descriptorRange;
};

using VisibilityGpuConstantBufferTablePair = std::pair<D3D12_SHADER_VISIBILITY, Material_gpu_constant_buffer_table>;
using VisibilityGpuConstantBufferTablePairs = std::vector<VisibilityGpuConstantBufferTablePair>;
struct Material_gpu_constant_buffers {
  // Owns the buffer data in the following tables. Not really intended for anything other than a container for cleanup
  // later.
  std::vector<std::shared_ptr<Gpu_buffer_reference>> gpuBuffers;

  VisibilityGpuConstantBufferTablePairs tables;
};

class Material_context_impl;

class D3D12_material_manager : public Material_manager {
 public:
  D3D12_material_manager(
          IAsset_manager& assetManager, oe::pipeline_d3d12::D3D12_texture_manager& textureManager,
          ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
          D3D12_device_resources& deviceResources);
  ~D3D12_material_manager() override = default;

  D3D12_material_manager(const D3D12_material_manager& other) = delete;
  D3D12_material_manager(D3D12_material_manager&& other) = delete;
  void operator=(const D3D12_material_manager& other) = delete;
  void operator=(D3D12_material_manager&& other) = delete;

  static void initStatics();
  static void destroyStatics();


  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // IMaterial_manager implementation
  Material_context_handle createMaterialContext(const std::string& name) override;

  bool getMaterialStateIdentifier(Material_context_handle handle, Material_state_identifier& stateIdentifier) override;

  const std::string& name() const override { return _name; }

  void loadMaterialToContext(
          Material_context_handle materialContextHandle, const Material& material,
          const Material_state_identifier& stateIdentifier,
          const Material_compiler_inputs& materialCompilerInputs) override;

  void loadResourcesToContext(
          Material_context_handle materialContextHandle, const Material::Shader_resources& shader_resources,
          const std::vector<Vertex_attribute_element>& vsInputs) override;

  void loadPipelineStateToContext(
          Material_context_handle materialContextHandle, const Pipeline_state_inputs& inputs) override;

  void bindMaterialContextToDevice(Material_context_handle materialContext) override;

  bool isMaterialContextDataReady(Material_context_handle materialContext) const override;

  void setDataReady(Material_context_handle materialContextHandle, bool ready) override;

  void unbind() override;

  void updateLightBuffers() override {}

 private:
  void queueTextureLoad(const Texture& texture) override;

  static D3D12_PRIMITIVE_TOPOLOGY_TYPE getMeshDataTopology(Mesh_index_type meshIndexType);

  void compileShader(
          const Material::Shader_compile_settings& compileSettings, bool enableOptimizations,
          const std::string& shaderTarget, ID3DBlob** result) const;

  void createRootSignature(const oe::Material::Shader_resources& shaderResources, Material_context_impl& impl);
  void createMeshBufferViews(const std::vector<Vertex_attribute_element>& vsInputs, Material_context_impl& impl);

  Microsoft::WRL::ComPtr<ID3D12PipelineState> createPipelineState(const Material_context_impl& impl, const Pipeline_state_inputs& inputs);

  Material_context_impl* getMaterialContextImpl(Material_context_handle handle);
  const Material_context_impl* getMaterialContextImpl(Material_context_handle handle) const;

  IAsset_manager& _assetManager;
  D3D12_texture_manager& _textureManager;
  ILighting_manager& _lightingManager;
  IPrimitive_mesh_data_factory& _primitiveMeshDataFactory;
  D3D12_device_resources& _deviceResources;

  struct Device_dependent_fields {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;

    std::vector<std::shared_ptr<Material_context_impl>> materialContexts;
    ID3D12Device6* device = nullptr;

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    std::unordered_map<const Material*, std::unique_ptr<Material_gpu_constant_buffers>> perMaterialConstantBuffers;
  } _deviceDependent;

  static std::string _name;
  static std::string _shaderTargetVertex;
  static std::string _shaderTargetPixel;
  D3D12_DEPTH_STENCIL_DESC getD12DepthStencilDesc(const Depth_stencil_config& depthStencilConfig) const;
};
}// namespace oe::pipeline_d3d12