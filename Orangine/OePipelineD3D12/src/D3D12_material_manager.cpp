#include "D3D12_material_manager.h"

#include "D3D12_renderer_types.h"
#include "PipelineUtils.h"
#include "Constant_buffer_pool.h"
#include "Frame_resources.h"

#include <OeCore/Mesh_utils.h>

#include <CommonStates.h>
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
        Manager_instance<IMaterial_manager>& out, IAsset_manager& assetManager,
        oe::pipeline_d3d12::D3D12_texture_manager& textureManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        oe::pipeline_d3d12::D3D12_device_resources& deviceResources)
{
  out = Manager_instance<IMaterial_manager>(std::make_unique<oe::pipeline_d3d12::D3D12_material_manager>(
          assetManager, textureManager, lightingManager, primitiveMeshDataFactory, deviceResources));
}

namespace oe::pipeline_d3d12 {

std::string D3D12_material_manager::_name;
std::string D3D12_material_manager::_shaderTargetVertex;
std::string D3D12_material_manager::_shaderTargetPixel;

using VisibilityTestFn = std::function<bool(const VisibilityGpuConstantBufferTablePair&)>;
constexpr auto kIsVertexVisibleConstantBuffer = [](const VisibilityGpuConstantBufferTablePair& buffer) {
  OE_CHECK(buffer.first != D3D12_SHADER_VISIBILITY_ALL);
  return buffer.first == D3D12_SHADER_VISIBILITY_VERTEX;
};
constexpr auto kIsPixelVisibleConstantBuffer = [](const VisibilityGpuConstantBufferTablePair& buffer) {
  OE_CHECK(buffer.first != D3D12_SHADER_VISIBILITY_ALL);
  return buffer.first == D3D12_SHADER_VISIBILITY_PIXEL;
};

std::string createErrorBlobString(HRESULT hr, ID3D10Blob* errorMessage)
{
  std::stringstream ss;

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
std::string
createShaderErrorString(HRESULT hr, ID3D10Blob* errorMessage, const Material::Shader_compile_settings& compileSettings)
{
  std::stringstream ss;

  const auto shaderFilenameUtf8 = compileSettings.filename;
  ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

  ss << createErrorBlobString(hr, errorMessage);
  return ss.str();
}

D3D12_material_manager::D3D12_material_manager(
        oe::IAsset_manager& assetManager, oe::pipeline_d3d12::D3D12_texture_manager& textureManager,
        oe::ILighting_manager& lightingManager, oe::IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        D3D12_device_resources& deviceResources)
    : Material_manager(assetManager)
    , _assetManager(assetManager)
    , _textureManager(textureManager)
    , _lightingManager(lightingManager)
    , _primitiveMeshDataFactory(primitiveMeshDataFactory)
    , _deviceResources(deviceResources)
{
}

void D3D12_material_manager::initStatics()
{
  _name = "D3D12_material_manager";
  _shaderTargetVertex = "vs_5_0";
  _shaderTargetPixel = "ps_5_0";
}

void D3D12_material_manager::destroyStatics() {}

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

  _deviceDependent.commandList = _deviceResources.GetPipelineCommandList();
}

void D3D12_material_manager::destroyDeviceDependentResources() { _deviceDependent = {}; }

std::weak_ptr<Material_context> D3D12_material_manager::createMaterialContext(const std::string& name)
{
  auto context = std::make_shared<Material_context_impl>();
  context->name = oe::utf8_decode(name);

  _deviceDependent.materialContexts.push_back(context);
  return std::static_pointer_cast<Material_context>(context);
}

void D3D12_material_manager::loadMaterialToContext(
        const Material& material, Material_context& materialContext, bool enableOptimizations)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  createVertexShader(enableOptimizations, material, impl);
  createPixelShader(enableOptimizations, material, impl);
  impl.constantBufferCount = material.getShaderConstantLayout().constantBuffers.size();
  //createStaticConstantBuffers(material, impl);
  impl.cullMode = material.faceCullMode();
}

void D3D12_material_manager::loadResourcesToContext(
        const Material::Shader_resources& shaderResources,
        const std::vector<Vertex_attribute_element>& vsInputs,
        Material_context& materialContext)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  createRootSignature(shaderResources, impl);
  createMeshBufferViews(vsInputs, impl);
}

void D3D12_material_manager::loadPipelineStateToContext(Material_context& materialContext)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  createPipelineState(impl);
}

void D3D12_material_manager::createVertexShader(
        bool enableOptimizations, const Material& material, Material_context_impl& materialContext) const
{
  // Generate a settings object from the given flags
  auto compileSettings = material.vertexShaderSettings(materialContext.compilerInputs.flags);
  debugLogSettings("vertex shader", compileSettings);

  compileShader(compileSettings, _shaderTargetVertex, materialContext.vertexShader.ReleaseAndGetAddressOf());
}

void D3D12_material_manager::createPixelShader(
        bool enableOptimizations, const Material& material, Material_context_impl& materialContext) const
{
  // Generate a settings object from the given flags
  auto compileSettings = material.pixelShaderSettings(materialContext.compilerInputs.flags);
  debugLogSettings("pixel shader", compileSettings);

  compileShader(compileSettings, _shaderTargetPixel, materialContext.pixelShader.ReleaseAndGetAddressOf());

  materialContext.rtvFormats.clear();
  OE_CHECK(material.getShaderOutputLayout().renderTargetCountFormats.size() < 8);
  OE_CHECK(!material.getShaderOutputLayout().renderTargetCountFormats.empty());
  materialContext.rtvFormats.assign(
          material.getShaderOutputLayout().renderTargetCountFormats.begin(),
          material.getShaderOutputLayout().renderTargetCountFormats.end());
}

void D3D12_material_manager::createStaticConstantBuffers(const Material& material, Material_context_impl& materialContext)
{
  // Create the constant buffers that are specific to this material instance
  // (i.e. shared for all renders using this material)
  {
    auto matPos = _deviceDependent.perMaterialConstantBuffers.find(&material);
    if (matPos == _deviceDependent.perMaterialConstantBuffers.end()) {
      Shader_constant_buffer_usage usage = Shader_constant_buffer_usage::Per_material;
      auto constantBuffers = std::make_unique<Material_gpu_constant_buffers>();
      createStaticConstantBuffersForUsage(material, usage, *constantBuffers);
      auto insertRes = _deviceDependent.perMaterialConstantBuffers.insert({&material, std::move(constantBuffers)});
      OE_CHECK(insertRes.second == true);
      matPos = insertRes.first;
    }

    materialContext.perMaterialConstantBuffers = matPos->second.get();
  }

  // Create per-draw constant buffers
  {
    if (!materialContext.perDrawConstantBuffersInitialized) {
      Shader_constant_buffer_usage usage = Shader_constant_buffer_usage::Per_draw;
      createStaticConstantBuffersForUsage(material, usage, materialContext.perDrawConstantBuffers);
      materialContext.perDrawConstantBuffersInitialized = true;
    }
  }

  // TODO: Skelington constant buffers
}

void D3D12_material_manager::createStaticConstantBuffersForUsage(
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

      auto gpuBufferReference = std::make_shared<Gpu_buffer_reference>();
      size_t paddedBufferSizeBytes = 256 + (bufferInfo.sizeInBytes / 256) * 256;
      gpuBufferReference->gpuBuffer = Gpu_buffer::create(_deviceDependent.device, bufferName, paddedBufferSizeBytes);
      gpuBufferReference->cpuBuffer = std::make_shared<Mesh_buffer_accessor>(
              std::make_shared<Mesh_buffer>(paddedBufferSizeBytes), 1U, static_cast<uint32_t>(paddedBufferSizeBytes),
              0U);
      memset(gpuBufferReference->cpuBuffer->buffer->data, 0, gpuBufferReference->cpuBuffer->buffer->dataSize);
      constantBuffers.gpuBuffers.push_back(gpuBufferReference);

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
                [d3dvisibility](const VisibilityGpuConstantBufferTablePair& tablePair) {
                  return tablePair.first == d3dvisibility;
                });
        if (pos == constantBuffers.tables.end()) {
          constantBuffers.tables.emplace_back(d3dvisibility, Material_gpu_constant_buffer_table{});
          pos = constantBuffers.tables.end() - 1;
        }
        pos->second.buffers.push_back(gpuBufferReference.get());
      }

      constantBuffers.gpuBuffers.push_back(std::move(gpuBufferReference));
    }

    // Now we know how many descriptors are needed in each range, create them.
    for (auto& tablePair : constantBuffers.tables) {
      auto& table = tablePair.second;
      if (table.buffers.empty()) {
        continue;
      }

      table.descriptorRange = _deviceResources.getSrvHeap().allocateRange(table.buffers.size());

      auto cpuHandle = table.descriptorRange.cpuHandle;
      for (const auto& buffer : table.buffers) {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        uint32_t bufferSizeBytes = buffer->gpuBuffer->getBufferSizeBytes();
        OE_CHECK(bufferSizeBytes % 4 == 0 && bufferSizeBytes != 0);
        desc.Buffer.NumElements = (UINT) bufferSizeBytes / 4U;
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

        auto constantBufferView = buffer->gpuBuffer->getAsConstantBufferView();
        _deviceResources.GetD3DDevice()->CreateConstantBufferView(&constantBufferView, cpuHandle);

        cpuHandle.Offset(1, table.descriptorRange.incrementSize);
      }
    }
  }
}

void D3D12_material_manager::compileShader(
        const Material::Shader_compile_settings& compileSettings, const std::string& shaderTarget,
        ID3DBlob** result) const
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
          compileSettings.entryPoint.c_str(), shaderTarget.c_str(), compileFlags, 0, result,
          errorMsgs.ReleaseAndGetAddressOf());
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
            D3D12_DESCRIPTOR_RANGE_TYPE_CBV, range.descriptorCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
            range.offsetInDescriptorsFromHeapStart);
  }
}

void D3D12_material_manager::createRootSignature(
        const oe::Material::Shader_resources& shaderResources, Material_context_impl& impl)
{
  using ranges::views::filter;

  // Parameters for the root signature
  // Layout (when fully loaded):
  //   0 : constant tables             (standard layout - populated outside of this class)
  //   2 : shader resource view table  (pixel visibility - optional, only if has textures)
  //   3 : sampler table               (pixel visibility - optional, only if has textures)
  std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> d3dConstantDescriptorRanges{};
  std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> d3dPsSrvDescriptorRanges{};
  std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> d3dPsSamplerDescriptorRanges{};
  std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
  impl.rootSignatureRootDescriptorTables.clear();

/*
OE_CHECK(impl.perMaterialConstantBuffers);
  std::vector<CD3DX12_DESCRIPTOR_RANGE1> d3dVsCbvDescriptorRanges;
  std::vector<CD3DX12_DESCRIPTOR_RANGE1> d3dPsCbvDescriptorRanges;
  ///////////////////////////////
  /// Create constant buffer descriptor tables
  // Ordering is per-material, then per-draw.
  appendDescriptorTablesToRange(
          impl.perMaterialConstantBuffers->tables, d3dVsCbvDescriptorRanges, kIsVertexVisibleConstantBuffer);
  appendDescriptorTablesToRange(
          impl.perDrawConstantBuffers.tables, d3dVsCbvDescriptorRanges, kIsVertexVisibleConstantBuffer);
  if (!d3dVsCbvDescriptorRanges.empty()) {
    // Using the heap start handle, as the range has offsetInDescriptorsFromTableStart defined as the number of descriptors from the start of the heap.
    impl.rootSignatureRootDescriptorTables.emplace_back(_deviceResources.getSrvHeap().getGpuDescriptorHandleForHeapStart());
    LOG(INFO) << "  Root[" << rootParameters.size()
              << "] = CBV Table for VS (#ranges=" << d3dVsCbvDescriptorRanges.size() << ")";
    rootParameters.emplace_back();
    rootParameters.back().InitAsDescriptorTable(
            d3dVsCbvDescriptorRanges.size(), d3dVsCbvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_VERTEX);
  }

  appendDescriptorTablesToRange(
          impl.perMaterialConstantBuffers->tables, d3dPsCbvDescriptorRanges, kIsPixelVisibleConstantBuffer);
  appendDescriptorTablesToRange(
          impl.perDrawConstantBuffers.tables, d3dPsCbvDescriptorRanges, kIsPixelVisibleConstantBuffer);
  if (!d3dPsCbvDescriptorRanges.empty()) {
    // Using the heap start handle, as the range has offsetInDescriptorsFromTableStart defined as the number of descriptors from the start of the heap.
    impl.rootSignatureRootDescriptorTables.emplace_back(_deviceResources.getSrvHeap().getGpuDescriptorHandleForHeapStart());
    LOG(INFO) << "  Root[" << rootParameters.size()
              << "] = CBV Table for PS (#ranges=" << d3dPsCbvDescriptorRanges.size() << ")";
    rootParameters.emplace_back();
    rootParameters.back().InitAsDescriptorTable(
            d3dPsCbvDescriptorRanges.size(), d3dPsCbvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
  }
  */
  ///////////////////////////////
  /// Create a placeholder for constants at slot 0. This is populated by whomever calls DrawInstance
  {
    OE_CHECK(impl.constantBufferCount > 0);
    d3dConstantDescriptorRanges[0] = {
            D3D12_DESCRIPTOR_RANGE_TYPE_CBV, impl.constantBufferCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};

    // Don't know the address - just put a dummy value in here.
    impl.rootSignatureRootDescriptorTables.emplace_back(D3D12_GPU_DESCRIPTOR_HANDLE{});
    rootParameters.emplace_back();
    rootParameters.back().InitAsDescriptorTable(
            d3dConstantDescriptorRanges.size(), d3dConstantDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);
  }

  ///////////////////////////////
  /// Create SRV descriptor tables
  uint32_t numSrvDescriptors = shaderResources.textures.size();
  if (numSrvDescriptors) {
    Descriptor_range range = _deviceResources.getSrvHeap().allocateRange(numSrvDescriptors);
    auto cpuHandle = range.cpuHandle;
    for (const auto& texture : shaderResources.textures) {
      ID3D12Resource* resource = _textureManager.getResource(*texture);
      auto resourceDesc = resource->GetDesc();
      OE_CHECK(resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      desc.Format = resourceDesc.Format;
      if (texture->isCubeMap()) {
        desc.ViewDimension =
                texture->isArray() ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;
      }
      else if (texture->isArray()) {
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
      }
      else {
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      }
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      desc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
      if (texture->isArraySlice()) {
        desc.Texture2D.PlaneSlice = texture->arraySlice();
      }

      _deviceResources.GetD3DDevice()->CreateShaderResourceView(resource, &desc, cpuHandle);
      cpuHandle.Offset(1, range.incrementSize);
    }

    d3dPsSrvDescriptorRanges[0] = {
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV, range.descriptorCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};

    impl.rootSignatureRootDescriptorTables.emplace_back(range.gpuHandle);

    LOG(INFO) << "  Root[" << rootParameters.size() << "] = SRV Table (size=" << range.descriptorCount << ")";
    rootParameters.emplace_back();
    rootParameters.back().InitAsDescriptorTable(
            d3dPsSrvDescriptorRanges.size(), d3dPsSrvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
  }

  ///////////////////////////////
  /// Sampler descriptors
  // TODO: We can probably change this to use static samplers. We are allowed up to 2032 of them, should be more than enough.
  // Create descriptor table for sampler states
  uint32_t numSamplerDescriptors = shaderResources.samplerDescriptors.size();
  if (numSamplerDescriptors > 0) {
    // Only create sampler states if they don't exist.
    if (impl.samplerDescriptorTable.descriptorCount == 0) {
      if (impl.samplerDescriptorTable.isValid()) {
        _deviceResources.getSamplerHeap().releaseRange(impl.samplerDescriptorTable);
      }

      // Create a descriptor table for the samplers
      impl.samplerDescriptorTable = _deviceResources.getSamplerHeap().allocateRange(numSamplerDescriptors);

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
    d3dPsSamplerDescriptorRanges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSamplerDescriptors, 0, 0};
    OE_CHECK(impl.samplerDescriptorTable.descriptorCount == numSamplerDescriptors);

    impl.rootSignatureRootDescriptorTables.emplace_back(impl.samplerDescriptorTable.gpuHandle);

    LOG(INFO) << "  Root[" << rootParameters.size() << "] = Sampler Table (size=" << numSamplerDescriptors << ")";
    rootParameters.emplace_back();
    rootParameters.back().InitAsDescriptorTable(
            d3dPsSamplerDescriptorRanges.size(), d3dPsSamplerDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
  }

  // Create the root signature.
  {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC::Init_1_1(
            rootSignatureDesc, rootParameters.size(), rootParameters.data(), 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3D10Blob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT res = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error);
    if (FAILED(res) && error) {
      std::string errMsg = createErrorBlobString(res, error.Get());
      LOG(WARNING) << errMsg;
      ThrowIfFailed(res);
    }
    ThrowIfFailed(_deviceDependent.device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&impl.rootSignature)));

    std::wstring debugName = impl.name + L" (CompiledMaterial=" + oe::utf8_decode(impl.compilerInputs.name) + L")";
    SetObjectName(impl.rootSignature.Get(), debugName.c_str());
  }
}

void D3D12_material_manager::createMeshBufferViews(
        const std::vector<Vertex_attribute_element>& vsInputs,
        Material_context_impl& impl)
{
  // Define the vertex input layout.
  LOG(G3LOG_DEBUG) << "Adding vertex attributes";

  impl.inputElementDescs.clear();
  impl.inputElementDescs.reserve(impl.compilerInputs.vsInputs.size());
  for (auto inputSlot = 0U; inputSlot < impl.compilerInputs.vsInputs.size(); ++inputSlot) {
    const auto attrSemantic = impl.compilerInputs.vsInputs[inputSlot];
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
    impl.inputElementDescs.push_back(desc);
  }
}

void D3D12_material_manager::createPipelineState(Material_context_impl& impl)
{
  // Describe and create the graphics pipeline state object (PSO).

  // Create the PSO
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = {impl.inputElementDescs.data(), static_cast<UINT>(impl.inputElementDescs.size())};
  psoDesc.pRootSignature = impl.rootSignature.Get();
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(impl.vertexShader.Get());
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(impl.pixelShader.Get());
  psoDesc.PrimitiveTopologyType = getMeshDataTopology(impl.compilerInputs.meshIndexType);
  psoDesc.SampleMask = UINT_MAX;// TODO
  psoDesc.SampleDesc.Count = 1; // TODO - set quality and sample counts

  // Rasterizer state
  if (impl.pipelineStateInputs.wireframe) {
    psoDesc.RasterizerState = DirectX::CommonStates::Wireframe;
  }
  else if (impl.cullMode == Material_face_cull_mode::Back_face) {
    psoDesc.RasterizerState = DirectX::CommonStates::CullClockwise;
  }
  else if (impl.cullMode == Material_face_cull_mode::Front_face) {
    psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
  }
  else if (impl.cullMode == Material_face_cull_mode::None) {
    psoDesc.RasterizerState = DirectX::CommonStates::CullNone;
  }
  else {
    OE_THROW(std::invalid_argument("Invalid cull mode"));
  }

  // Blend state
  switch (impl.compilerInputs.depthStencilConfig.getBlendMode()) {
    case (Render_pass_blend_mode::Opaque):
      psoDesc.BlendState = DirectX::CommonStates::Opaque;
      break;
    case (Render_pass_blend_mode::Blended_alpha):
      psoDesc.BlendState = DirectX::CommonStates::AlphaBlend;
      break;
    case (Render_pass_blend_mode::Additive):
      psoDesc.BlendState = DirectX::CommonStates::Additive;
      break;
    default:
      OE_THROW(std::runtime_error("Invalid blend state"));
  }

  // Depth/Stencil
  psoDesc.DepthStencilState = getD12DepthStencilDesc(impl.compilerInputs.depthStencilConfig);
  if (psoDesc.DepthStencilState.DepthEnable || psoDesc.DepthStencilState.StencilEnable) {
    psoDesc.DSVFormat = _deviceResources.getDepthStencilFormat();
  }

  // Render targets
  psoDesc.NumRenderTargets = impl.rtvFormats.size();
  for (size_t idx = 0; idx < impl.rtvFormats.size(); ++idx) {
    psoDesc.RTVFormats[idx] = impl.rtvFormats[idx];
    if (idx > 0) {
      // CommonStates does not set render target states other than 0.
      psoDesc.BlendState.RenderTarget[idx] = psoDesc.BlendState.RenderTarget[0];
    }
  }

  ThrowIfFailed(_deviceDependent.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&impl.pipelineState)));
}
D3D12_DEPTH_STENCIL_DESC
D3D12_material_manager::getD12DepthStencilDesc(const Depth_stencil_config& depthStencilConfig) const
{
  D3D12_DEPTH_STENCIL_DESC desc;
  Render_pass_depth_mode depthMode = depthStencilConfig.getDepthMode();
  const auto depthReadEnabled =
          depthMode == Render_pass_depth_mode::Read_only || depthMode == Render_pass_depth_mode::Read_write;
  const auto depthWriteEnabled =
          depthMode == Render_pass_depth_mode::Write_only || depthMode == Render_pass_depth_mode::Read_write;

  desc.DepthEnable = depthReadEnabled || depthWriteEnabled;
  desc.DepthWriteMask = depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  desc.DepthFunc = depthReadEnabled ? D3D12_COMPARISON_FUNC_LESS_EQUAL : D3D12_COMPARISON_FUNC_ALWAYS;

  desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
  if (Render_pass_stencil_mode::Disabled == depthStencilConfig.getStencilMode()) {
    desc.StencilEnable = false;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
  }
  else {
    desc.StencilEnable = true;
    desc.StencilReadMask = depthStencilConfig.getStencilReadMask();
    desc.StencilWriteMask = depthStencilConfig.getStencilWriteMask();
    desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
  }

  // Backface same as front
  desc.BackFace = desc.FrontFace;
  return desc;
}

void D3D12_material_manager::bindMaterialContextToDevice(const Material_context& materialContext)
{
  const Material_context_impl& impl = getMaterialContextImpl(materialContext);

  _deviceResources.getCurrentFrameResources().setCurrentBoundRootSignature(impl.rootSignature.Get());
  _deviceDependent.commandList->SetGraphicsRootSignature(impl.rootSignature.Get());
  for (uint32_t index = 0; index < impl.rootSignatureRootDescriptorTables.size(); ++index) {
    if (index == Frame_resources::kConstantBufferRootSignatureIndex) {
      continue;
    }
    _deviceDependent.commandList->SetGraphicsRootDescriptorTable(
            index, impl.rootSignatureRootDescriptorTables.at(index));
  }
  _deviceDependent.commandList->SetPipelineState(impl.pipelineState.Get());
}

void D3D12_material_manager::unbind()
{
  Material_manager::unbind();
  _deviceResources.getCurrentFrameResources().setCurrentBoundRootSignature(nullptr);
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
