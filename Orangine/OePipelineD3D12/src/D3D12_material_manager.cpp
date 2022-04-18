#include "D3D12_material_manager.h"

#include "D3D12_renderer_types.h"
#include "PipelineUtils.h"

#include <OeCore/Mesh_utils.h>

#include <cinttypes>
#include <comdef.h>
#include <d3dcompiler.h>
#include <d3dx12/d3dx12.h>
#include <range/v3/view.hpp>

using Microsoft::WRL::ComPtr;
using oe::Material_context;
using oe::Mesh_gpu_data;

template<>
void oe::create_manager(
        Manager_instance<IMaterial_manager>& out, IAsset_manager& assetManager, ITexture_manager& textureManager,
        ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        oe::pipeline_d3d12::D3D12_device_resources& deviceResources)
{
  out = Manager_instance<IMaterial_manager>(std::make_unique<oe::pipeline_d3d12::D3D12_material_manager>(
          assetManager, textureManager, lightingManager, primitiveMeshDataFactory, deviceResources));
}

namespace oe::pipeline_d3d12 {

using VisibilityTestFn = std::function<bool(const VisibilityGpuConstantBufferTablePair&)>;
constexpr auto kIsVertexVisibleConstantBuffer = [](const VisibilityGpuConstantBufferTablePair& buffer) {
  OE_CHECK((D3D12_SHADER_VISIBILITY_ALL & D3D12_SHADER_VISIBILITY_ALL) != D3D12_SHADER_VISIBILITY_ALL);
  return (buffer.first & D3D12_SHADER_VISIBILITY_VERTEX) == D3D12_SHADER_VISIBILITY_VERTEX;
};
constexpr auto kIsPixelVisibleConstantBuffer = [](const VisibilityGpuConstantBufferTablePair& buffer) {
  OE_CHECK((D3D12_SHADER_VISIBILITY_ALL & D3D12_SHADER_VISIBILITY_ALL) != D3D12_SHADER_VISIBILITY_ALL);
  return (buffer.first & D3D12_SHADER_VISIBILITY_PIXEL) == D3D12_SHADER_VISIBILITY_PIXEL;
};

static constexpr size_t kRootSignatureIdx_SrvVertex = 0;
static constexpr size_t kRootSignatureIdx_SrvPixel = 1;
static constexpr size_t kRootSignatureIdx_Sampler = 2;

std::string
createShaderErrorString(HRESULT hr, ID3D10Blob* errorMessage, const Material::Shader_compile_settings& compileSettings)
{
  std::stringstream ss;

  const auto shaderFilenameUtf8 = compileSettings.filename;
  ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

  // Get a pointer to the error message text buffer.
  if (errorMessage != nullptr) {
    const auto compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());
    ss << compileErrors << std::endl;
  }
  else {
    const _com_error err(hr);
    ss << utf8_encode(std::wstring(err.ErrorMessage()));
  }

  return ss.str();
}

D3D12_material_manager::D3D12_material_manager(
        oe::IAsset_manager& assetManager, oe::ITexture_manager& textureManager, oe::ILighting_manager& lightingManager,
        oe::IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources)
    : Material_manager(assetManager)
    , _assetManager(assetManager)
    , _textureManager(textureManager)
    , _lightingManager(lightingManager)
    , _primitiveMeshDataFactory(primitiveMeshDataFactory)
    , _deviceResources(deviceResources)
{}

void D3D12_material_manager::createDeviceDependentResources()
{
  _deviceDependent.device = _deviceResources.GetD3DDevice();
  OE_CHECK(_deviceDependent.device);

  // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
  _deviceDependent.featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

  if (FAILED(_deviceDependent.device->CheckFeatureSupport(
              D3D12_FEATURE_ROOT_SIGNATURE, &_deviceDependent.featureData, sizeof(_deviceDependent.featureData))))
  {
    _deviceDependent.featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDescriptorPoolSize, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};

  _deviceDependent.srvHeap = Descriptor_heap_pool{_deviceDependent.device, srvHeapDesc, "MaterialManager.srvHeap"};

  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kDescriptorPoolSize, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  _deviceDependent.samplerHeap =
          Descriptor_heap_pool{_deviceDependent.device, samplerHeapDesc, "MaterialManager.samplerHeap"};

  _deviceDependent.commandList = _deviceResources.GetPipelineCommandList();
}

void D3D12_material_manager::destroyDeviceDependentResources()
{
  _deviceDependent = {};
}

std::weak_ptr<Material_context> D3D12_material_manager::createMaterialContext()
{
  _deviceDependent.materialContexts.push_back(std::make_shared<Material_context_impl>());
  return std::static_pointer_cast<Material_context>(_deviceDependent.materialContexts.back());
}

void D3D12_material_manager::createVertexShader(
        bool enableOptimizations, const Material& material, Material_context& materialContext) const
{
  // Generate a settings object from the given flags
  auto compileSettings = material.vertexShaderSettings(materialContext.compilerInputs.flags);
  debugLogSettings("vertex shader", compileSettings);

  auto& impl = getMaterialContextImpl(materialContext);
  compileShader(compileSettings, impl.vertexShader.ReleaseAndGetAddressOf());
}

void D3D12_material_manager::createPixelShader(
        bool enableOptimizations, const Material& material, Material_context& materialContext) const
{
  // Generate a settings object from the given flags
  auto compileSettings = material.pixelShaderSettings(materialContext.compilerInputs.flags);
  debugLogSettings("pixel shader", compileSettings);

  auto& impl = getMaterialContextImpl(materialContext);
  compileShader(compileSettings, impl.pixelShader.ReleaseAndGetAddressOf());
}

void D3D12_material_manager::createConstantBuffers(const Material& material, Material_context& materialContext)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  // Create the constant buffers that are specific to this material instance
  // (i.e. shared for all renders using this material)
  {
    auto matPos = _deviceDependent.perMaterialConstantBuffers.find(&material);
    if (matPos == _deviceDependent.perMaterialConstantBuffers.end()) {
      Shader_constant_buffer_usage usage = Shader_constant_buffer_usage::Per_material;
      auto constantBuffers = std::make_unique<Material_gpu_constant_buffers>();
      createConstantBuffersForUsage(material, usage, *constantBuffers);
      auto insertRes = _deviceDependent.perMaterialConstantBuffers.insert({&material, std::move(constantBuffers)});
      OE_CHECK(insertRes.second == true);
      matPos = insertRes.first;
    }

    impl.perMaterialConstantBuffers = matPos->second.get();
  }

  // Create per-draw constant buffers
  {
    if (!impl.perDrawConstantBuffersInitialized) {
      Shader_constant_buffer_usage usage = Shader_constant_buffer_usage::Per_draw;
      createConstantBuffersForUsage(material, usage, impl.perDrawConstantBuffers);
      impl.perDrawConstantBuffersInitialized = true;
    }
  }

  // TODO: Skelington constant buffers
}

void D3D12_material_manager::createConstantBuffersForUsage(
        const Material& material, const Shader_constant_buffer_usage& usage,
        Material_gpu_constant_buffers& constantBuffers)
{
  {
    std::wstring bufferName{};
    OE_CHECK(constantBuffers.gpuBuffers.empty());
    for (const auto& bufferInfo : material.getShaderConstantLayout().constantBuffers) {
      if (bufferInfo.usage != usage) {
        continue;
      }

#if !defined(NDEBUG)
      bufferName = utf8_decode(
              material.materialType() + " " + shaderConstantBufferUsageToString(usage) + " " +
              shaderConstantBufferVisibilityToString(bufferInfo.visibility) + " CB r" +
              std::to_string(bufferInfo.registerIndex));
#endif

      auto gpuBuffer = Gpu_buffer::create(_deviceDependent.device, bufferName, bufferInfo.sizeInBytes);
      constantBuffers.gpuBuffers.push_back(std::move(gpuBuffer));

      static constexpr auto numVisibility =
              static_cast<uint32_t>(Shader_constant_buffer_visibility::Num_shader_constant_buffer_visibility);
      for (auto visibilityIdx = 0; visibilityIdx < numVisibility; visibilityIdx++) {
        const auto visibility = static_cast<Shader_constant_buffer_visibility>(visibilityIdx);
        if (bufferInfo.visibility != Shader_constant_buffer_visibility::All && bufferInfo.visibility != visibility) {
          continue;
        }

        D3D12_SHADER_VISIBILITY d3dvisibility = toD3dShaderVisibility(visibility);
        auto pos = std::find_if(
                constantBuffers.tables.begin(), constantBuffers.tables.end(),
                [d3dvisibility](const VisibilityGpuConstantBufferTablePair& tablePair) { return tablePair.first == d3dvisibility; });
        if (pos == constantBuffers.tables.end()) {
          constantBuffers.tables.emplace_back(d3dvisibility, Material_gpu_constant_buffer_table{});
          pos = constantBuffers.tables.end() - 1;
        }
        pos->second.buffers.push_back(gpuBuffer.get());
      }

      constantBuffers.gpuBuffers.push_back(std::move(gpuBuffer));
    }

    // Now we know how many descriptors are needed in each range, create them.
    for (auto& tablePair : constantBuffers.tables) {
      auto& table = tablePair.second;
      if (table.buffers.empty()) {
        continue;
      }

      table.descriptorRange = _deviceDependent.srvHeap.allocateRange(table.buffers.size());
    }
  }
}

void D3D12_material_manager::compileShader(
        const Material::Shader_compile_settings& compileSettings, ID3DBlob** result) const
{
  // Compiler defines
  std::vector<D3D_SHADER_MACRO> defines;
  for (const auto& define : compileSettings.defines) {
    defines.push_back({define.first.c_str(), define.second.c_str()});
  }
  // Null terminated array
  defines.push_back({nullptr, nullptr});

#if !defined(NDEBUG)
  // Enable better shader debugging with the graphics debugging tools.
  uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#else
  uint32_t compileFlags = D3DCOMPILE_ENABLE_STRICTNESS
#endif

  const auto filename = shaderPath() + "/" + compileSettings.filename;
  LOG(DEBUG) << "Compiling shader " << filename.c_str();
  Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
  HRESULT hr = D3DCompileFromFile(
          utf8_decode(filename).c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
          compileSettings.entryPoint.c_str(), "vs_5_0", compileFlags, 0, result, errorMsgs.ReleaseAndGetAddressOf());
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error(createShaderErrorString(hr, errorMsgs.Get(), compileSettings)));
  }
}

void appendDescriptorTablesToRange(
        const VisibilityGpuConstantBufferTablePairs& tables, std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges,
        const VisibilityTestFn& visilibityFn)
{
  // Get the descriptor table containing all the vertex visible constant buffers, if one exists.
  auto constantBufferTablePos = std::find_if(tables.begin(), tables.end(), visilibityFn);
  if (constantBufferTablePos != tables.end()) {
    const auto& range = constantBufferTablePos->second.descriptorRange;
    descriptorRanges.emplace_back(
            D3D12_DESCRIPTOR_RANGE_TYPE_CBV, range.descriptorCount, 0, /* base shader register... ? */
            0,                                                         /* registerSpace */
            D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
  }
}

void D3D12_material_manager::loadShaderResourcesToContext(
        const oe::Material::Shader_resources& shaderResources, Material_context& materialContext)
{
  using ranges::views::filter;
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  // Parameters for the root signature
  // Layout:
  //   0 : constants/SRV tables    (vertex visibility)
  //   1 : constants/SRV tables    (pixel visibility)
  //   2 : sampler table           (pixel visibility)
  std::array<CD3DX12_ROOT_PARAMETER1, 3> rootParameters{};

  // Descriptor tables for the Constant buffers are already created - just need to configure them in the root signature.
  // Shader Resource Views aren't created yet, that's a TODO right thar capt'n.

  // TODO: Upload constant buffer data to Gpu_buffer
  {
    if (impl.srvDescriptorTable.descriptorCount == 0) {
      // Ordering is per-material, then per-draw.

      OE_CHECK(impl.perMaterialConstantBuffers);

      std::vector<CD3DX12_DESCRIPTOR_RANGE1> d3dVsDescriptorRanges;
      appendDescriptorTablesToRange(
              impl.perMaterialConstantBuffers->tables, d3dVsDescriptorRanges, kIsVertexVisibleConstantBuffer);
      appendDescriptorTablesToRange(
              impl.perDrawConstantBuffers.tables, d3dVsDescriptorRanges, kIsVertexVisibleConstantBuffer);
      rootParameters[kRootSignatureIdx_SrvVertex].InitAsDescriptorTable(
              d3dVsDescriptorRanges.size(), d3dVsDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_VERTEX);

      std::vector<CD3DX12_DESCRIPTOR_RANGE1> d3dPsDescriptorRanges;
      appendDescriptorTablesToRange(
              impl.perMaterialConstantBuffers->tables, d3dPsDescriptorRanges, kIsPixelVisibleConstantBuffer);
      appendDescriptorTablesToRange(
              impl.perDrawConstantBuffers.tables, d3dPsDescriptorRanges, kIsPixelVisibleConstantBuffer);
      rootParameters[kRootSignatureIdx_SrvPixel].InitAsDescriptorTable(
              d3dPsDescriptorRanges.size(), d3dPsDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
    }

    // TODO: Same as above, but for SRV
  }

  // TODO: We can probably change this to use static samplers. We are allowed up to 2032 of them, should be more than enough.
  // Create descriptor table for sampler states
  {
    // Only create sampler states if they don't exist.
    uint32_t numDescriptors = shaderResources.samplerDescriptors.size();
    if (impl.samplerDescriptorTable.descriptorCount == 0 && numDescriptors > 0) {
      _deviceDependent.samplerHeap.releaseRange(impl.samplerDescriptorTable);

      // Create a descriptor table for the samplers
      impl.samplerDescriptorTable = _deviceDependent.samplerHeap.allocateRange(numDescriptors);

      CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = impl.samplerDescriptorTable.cpuHandle;
      int singleDescriptorOffset = static_cast<int>(impl.samplerDescriptorTable.incrementSize);
      for (size_t idx = 0; idx < shaderResources.samplerDescriptors.size(); ++idx) {
        const auto& descriptor = shaderResources.samplerDescriptors[idx];

        D3D12_SAMPLER_DESC samplerDesc{
                toD3DBasicFilter(descriptor.minFilter, descriptor.magFilter),
                toD3D12TextureAddressMode(descriptor.wrapU),
                toD3D12TextureAddressMode(descriptor.wrapV),
                toD3D12TextureAddressMode(descriptor.wrapW),
                0.0 /* mip bias */,
                16 /* max anisotropy */,
                toD3D12ComparisonFunc(descriptor.comparisonFunc),
                {0.0F, 0.0F, 0.0F, 0.0F} /* border color */,
                0.0F /* minLOD */,
                D3D12_FLOAT32_MAX /* maxLOD */};

        _deviceDependent.device->CreateSampler(&samplerDesc, cpuHandle);

        // Move the handle to the next slot in the table
        cpuHandle.Offset(singleDescriptorOffset);
      }
    }
    std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> d3dPsSrvDescriptorRanges{
            {{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numDescriptors, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC}}};
    OE_CHECK(impl.samplerDescriptorTable.descriptorCount == numDescriptors);
    rootParameters[kRootSignatureIdx_Sampler].InitAsDescriptorTable(
            d3dPsSrvDescriptorRanges.size(), d3dPsSrvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
  }

  // Create the root signature.
  {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC::Init_1_1(
            rootSignatureDesc, rootParameters.size(), rootParameters.data(), 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3D10Blob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
    ThrowIfFailed(_deviceDependent.device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&impl.rootSignature)));
  }

  // Describe and create the graphics pipeline state object (PSO).
  std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
  {
    // Define the vertex input layout.
    LOG(G3LOG_DEBUG) << "Adding vertex attributes";

    inputElementDescs.reserve(materialContext.compilerInputs.vsInputs.size());
    for (auto inputSlot = 0U; inputSlot < materialContext.compilerInputs.vsInputs.size(); ++inputSlot) {
      const auto attrSemantic = materialContext.compilerInputs.vsInputs[inputSlot];
      const auto semanticName = Vertex_attribute_meta::semanticName(attrSemantic.semantic.attribute);
      OE_CHECK(!semanticName.empty());
      LOG(DEBUG) << "  " << semanticName << " to slot " << inputSlot;

      D3D12_INPUT_ELEMENT_DESC desc{
              semanticName.data(),
              attrSemantic.semantic.semanticIndex,
              mesh_utils::getDxgiFormat(attrSemantic.type, attrSemantic.component),
              inputSlot,
              D3D12_APPEND_ALIGNED_ELEMENT,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
              0};
      inputElementDescs.push_back(desc);
    }

    // Create the PSO
    CD3DX12_BLEND_DESC blendDesc{D3D12_DEFAULT};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size())};
    psoDesc.pRootSignature = impl.rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(impl.vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(impl.pixelShader.Get());
    psoDesc.PrimitiveTopologyType = getMeshDataTopology(impl.compilerInputs.meshIndexType);

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);// TODO
    psoDesc.BlendState = blendDesc;                                  // TODO
    psoDesc.DepthStencilState.DepthEnable = FALSE;                   // TODO
    psoDesc.DepthStencilState.StencilEnable = FALSE;                 // TODO
    psoDesc.SampleMask = UINT_MAX;                                   // TODO
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;// TODO
    psoDesc.SampleDesc.Count = 1;                      // TODO

    ThrowIfFailed(_deviceDependent.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&impl.pipelineState)));
  }
}

void D3D12_material_manager::loadMeshGpuDataToContext(
        const Mesh_gpu_data& gpuData, const std::vector<Vertex_attribute_element>& vsInputs,
        Material_context& materialContext)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  // Vertex buffer views
  impl.vertexBufferViews.resize(materialContext.compilerInputs.vsInputs.size());
  auto viewIter = impl.vertexBufferViews.begin();
  for (const auto vsInput : materialContext.compilerInputs.vsInputs) {
    const auto vertexGpuBufferPos = gpuData.vertexBuffers.find(vsInput.semantic);
    OE_CHECK(vertexGpuBufferPos != gpuData.vertexBuffers.end());

    pipeline_d3d12::Gpu_buffer& vertexGpuBuffer = *vertexGpuBufferPos->second->gpuBuffer;
    *(viewIter++) = vertexGpuBuffer.GetAsVertexBufferView();
  }

  // Index buffer
  if (gpuData.indexBuffer) {
    impl.indexBufferView = gpuData.indexBuffer->GetAsIndexBufferView(gpuData.indexFormat);
  }
  else {
    impl.indexBufferView = {0U, 0U, DXGI_FORMAT_UNKNOWN};
  }
}

void D3D12_material_manager::render(
        const Material_context& materialContext, const Matrix4& worldMatrix,
        const Renderer_animation_data& rendererAnimationData, const Camera_data& camera)
{
  const Material_context_impl& impl = getMaterialContextImpl(materialContext);

  const auto numVertexViews = static_cast<uint32_t>(impl.vertexBufferViews.size());
  _deviceDependent.commandList->IASetVertexBuffers(0, numVertexViews, impl.vertexBufferViews.data());

  if (impl.indexBufferView.SizeInBytes > 0) {
    _deviceDependent.commandList->IASetIndexBuffer(&impl.indexBufferView);
  }
}

void D3D12_material_manager::bindMaterialContextToDevice(
        const Material_context& materialContext, bool enablePixelShader)
{
  const Material_context_impl& impl = getMaterialContextImpl(materialContext);

  OE_CHECK(impl.perDrawConstantBuffersInitialized && impl.perMaterialConstantBuffers);
  _deviceDependent.commandList->SetGraphicsRootSignature(impl.rootSignature.Get());
  _deviceDependent.commandList->SetGraphicsRootDescriptorTable(
          kRootSignatureIdx_SrvVertex, _deviceDependent.srvHeap.getGpuDescriptorHandleForHeapStart());
  _deviceDependent.commandList->SetGraphicsRootDescriptorTable(
          kRootSignatureIdx_SrvPixel, _deviceDependent.srvHeap.getGpuDescriptorHandleForHeapStart());
  _deviceDependent.commandList->SetGraphicsRootDescriptorTable(
          kRootSignatureIdx_Sampler, _deviceDependent.samplerHeap.getGpuDescriptorHandleForHeapStart());
  _deviceDependent.commandList->SetPipelineState(impl.pipelineState.Get());
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12_material_manager::getMeshDataTopology(Mesh_index_type meshIndexType)
{
  switch (meshIndexType) {
    case Mesh_index_type::Triangles:
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      break;
    case Mesh_index_type::Lines:
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
      break;
    default:
      OE_THROW(std::exception("Unsupported mesh topology"));
  }
}
}// namespace oe::pipeline_d3d12
