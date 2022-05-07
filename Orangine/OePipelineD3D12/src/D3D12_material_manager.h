#pragma once

#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"
#include "Descriptor_heap_pool.h"
#include "Gpu_buffer.h"

#include <OeCore/Material_context.h>
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

class Material_context_impl : public Material_context {
 public:
  Material_context_impl() = default;
  Material_context_impl(const Material_context_impl&) = delete;
  Material_context_impl(Material_context_impl&&) = default;
  Material_context_impl& operator=(const Material_context_impl&) = delete;
  Material_context_impl& operator=(Material_context_impl&&) = default;

  ~Material_context_impl() { releaseResources(); }

  void releaseResources() override { releasePipelineState(); };

  void releasePipelineState()
  {
    vertexShader.Reset();
    pixelShader.Reset();
    pipelineState.Reset();
    rootSignature.Reset();
  }

  Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
  Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
  Descriptor_range samplerDescriptorTable;

  std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
  uint32_t numVerticesPerInstance;
  D3D12_INDEX_BUFFER_VIEW indexBufferView;
  uint32_t numIndices;
  std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

  Material_gpu_constant_buffers perDrawConstantBuffers;
  Material_gpu_constant_buffers* perMaterialConstantBuffers = nullptr;
  bool perDrawConstantBuffersInitialized = false;
  Material_face_cull_mode cullMode;

  // Texture formats of the render target views.
  std::vector<DXGI_FORMAT> rtvFormats;

  std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> rootSignatureRootDescriptorTables;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

  // Optional: used primarily for debugging purposes.
  std::wstring name;
};

class D3D12_material_manager : public Material_manager {
 public:
  static constexpr uint32_t kDescriptorPoolSize = 100;
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
  std::weak_ptr<Material_context> createMaterialContext(const std::string& name) override;
  const std::string& name() const override { return _name; }

  void loadMaterialToContext(const Material& material, Material_context& materialContext, bool enableOptimizations);

  void loadResourcesToContext(
          const Material::Shader_resources& shaderResources, const Mesh_gpu_data& gpuData,
          const std::vector<Vertex_attribute_element>& vsInputs, Material_context& materialContext) override;

  void loadPipelineStateToContext(Material_context& materialContext) override;

  void bindMaterialContextToDevice(const Material_context& materialContext) override;

  void
  render(const Material_context& materialContext, const SSE::Matrix4& worldMatrix,
         const Renderer_animation_data& rendererAnimationData, const Camera_data& camera) override;

  void updateLightBuffers() override {}

 private:
  inline Material_context_impl& getMaterialContextImpl(Material_context& context) const
  {
    return static_cast<Material_context_impl&>(context);
  }

  inline const Material_context_impl& getMaterialContextImpl(const Material_context& context) const
  {
    return static_cast<const Material_context_impl&>(context);
  }

  static D3D12_PRIMITIVE_TOPOLOGY_TYPE getMeshDataTopology(Mesh_index_type meshIndexType);

  void compileShader(
          const Material::Shader_compile_settings& compileSettings, const std::string& shaderTarget,
          ID3DBlob** result) const;

  void createVertexShader(
          bool enableOptimizations, const Material& material, Material_context_impl& materialContext) const;

  void createPixelShader(
          bool enableOptimizations, const Material& material, Material_context_impl& materialContext) const;

  void createConstantBuffers(const Material& material, Material_context_impl& materialContext);

  // Adds mappings of shader visibility to buffer table. Any buffer visibilities on the material will be split up into
  // duplicate entries for both PIXEL and VERTEX D3D12 visibilities.
  void createConstantBuffersForUsage(
          const Material& material, const Shader_constant_buffer_usage& usage,
          Material_gpu_constant_buffers& constantBuffers);

  void createRootSignature(
          const oe::Material::Shader_resources& shaderResources, Material_context_impl& impl);
  void createMeshBufferViews(
          const Mesh_gpu_data& gpuData, const std::vector<Vertex_attribute_element>& vsInputs,
          Material_context_impl& impl);
  void createPipelineState(Material_context_impl& impl);

  IAsset_manager& _assetManager;
  D3D12_texture_manager& _textureManager;
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

  static std::string _name;
  static std::string _shaderTargetVertex;
  static std::string _shaderTargetPixel;
  D3D12_DEPTH_STENCIL_DESC getD12DepthStencilDesc(const Depth_stencil_config& depthStencilConfig) const;
};
}// namespace oe::pipeline_d3d12