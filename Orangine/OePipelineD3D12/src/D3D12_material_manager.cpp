#include "D3D12_material_manager.h"

#include "Constant_buffer_pool.h"
#include "D3D12_renderer_types.h"
#include "Frame_resources.h"
#include "PipelineUtils.h"

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

size_t materialContextHandleToIndex(Material_context_handle handle) { return static_cast<size_t>(handle - 1); }

Material_context_handle arrayIndexToMaterialContextHandle(size_t index)
{
  return static_cast<Material_context_handle>(index + 1);
}

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

class Material_context_impl {
 public:
  Material_context_impl() = default;
  Material_context_impl(const Material_context_impl&) = delete;
  Material_context_impl(Material_context_impl&&) = default;
  Material_context_impl& operator=(const Material_context_impl&) = delete;
  Material_context_impl& operator=(Material_context_impl&&) = default;

  Material_state_identifier stateIdentifier;

  Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
  Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
  Descriptor_range samplerDescriptorTable;

  std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
  D3D12_BLEND_DESC blendDesc;
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
  D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;

  uint32_t constantBufferCount = 0;
  Material_gpu_constant_buffers perDrawConstantBuffers;
  Material_face_cull_mode cullMode = {};

  // Texture formats of the render target views.
  std::vector<DXGI_FORMAT> rtvFormats;

  // Only used to create pipeline state objects - can be modified independently for the purposes of recreating only
  // the PSO
  struct {
    D3D12_RASTERIZER_DESC rasterizerDesc;
  } pipelineStateDesc;

  std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> rootSignatureRootDescriptorTables;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

  // Optional: used primarily for debugging purposes.
  std::wstring name;
  std::wstring materialName;

  bool isDataReady = false;
};

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
{}

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

Material_context_handle D3D12_material_manager::createMaterialContext(const std::string& name)
{
  auto context = std::make_shared<Material_context_impl>();
  context->name = oe::utf8_decode(name);
  Material_context_handle handle = arrayIndexToMaterialContextHandle(
          static_cast<Material_context_handle>(_deviceDependent.materialContexts.size()));

  _deviceDependent.materialContexts.push_back(context);
  return handle;
}

bool D3D12_material_manager::getMaterialStateIdentifier(
        Material_context_handle handle, Material_state_identifier& stateIdentifier)
{
  Material_context_impl* impl = getMaterialContextImpl(handle);
  if (!impl) {
    return false;
  }
  stateIdentifier = impl->stateIdentifier;
  return true;
}

void D3D12_material_manager::loadMaterialToContext(
        Material_context_handle materialContextHandle, const Material& material,
        const Material_state_identifier& stateIdentifier, const Material_compiler_inputs& materialCompilerInputs)
{
  Material_context_impl* impl = getMaterialContextImpl(materialContextHandle);
  OE_CHECK_MSG(impl != nullptr, "loadMaterialToContext: Invalid material context handle");

  // Create vertex shader
  {
    auto vertexCompileSettings = material.vertexShaderSettings(materialCompilerInputs.flags);
    debugLogSettings("vertex shader", vertexCompileSettings);
    compileShader(
            vertexCompileSettings, materialCompilerInputs.enableOptimizations, _shaderTargetVertex,
            impl->vertexShader.ReleaseAndGetAddressOf());
  }

  // Create pixel shader
  if (stateIdentifier.targetLayout != Render_pass_target_layout::None)
  {
    auto pixelCompileSettings = material.pixelShaderSettings(materialCompilerInputs.flags);
    debugLogSettings("pixel shader", pixelCompileSettings);
    compileShader(
            pixelCompileSettings, materialCompilerInputs.enableOptimizations, _shaderTargetPixel,
            impl->pixelShader.ReleaseAndGetAddressOf());

    impl->rtvFormats.clear();
    OE_CHECK(material.getShaderOutputLayout().renderTargetCountFormats.size() < 8);
    OE_CHECK(!material.getShaderOutputLayout().renderTargetCountFormats.empty());
    impl->rtvFormats.assign(
            material.getShaderOutputLayout().renderTargetCountFormats.begin(),
            material.getShaderOutputLayout().renderTargetCountFormats.end());
  }

  impl->constantBufferCount = material.getShaderConstantLayout().constantBuffers.size();
  impl->cullMode = material.faceCullMode();
  impl->stateIdentifier = stateIdentifier;
  impl->materialName = oe::utf8_decode(materialCompilerInputs.name);

  // Blend state
  switch (materialCompilerInputs.depthStencilConfig.getBlendMode()) {
    case (Render_pass_blend_mode::Opaque):
      impl->blendDesc = DirectX::CommonStates::Opaque;
      break;
    case (Render_pass_blend_mode::Blended_alpha):
      impl->blendDesc = DirectX::CommonStates::AlphaBlend;
      break;
    case (Render_pass_blend_mode::Additive):
      impl->blendDesc = DirectX::CommonStates::Additive;
      break;
    default:
      OE_THROW(std::runtime_error("Invalid blend state"));
  }

  impl->primitiveTopologyType = getMeshDataTopology(materialCompilerInputs.meshIndexType);

  // Depth Stencil State
  impl->depthStencilDesc = getD12DepthStencilDesc(materialCompilerInputs.depthStencilConfig);

  impl->stateIdentifier = stateIdentifier;
}

void D3D12_material_manager::loadResourcesToContext(
        Material_context_handle materialContextHandle, const Material::Shader_resources& shaderResources,
        const std::vector<Vertex_attribute_element>& vsInputs)
{
  Material_context_impl* impl = getMaterialContextImpl(materialContextHandle);
  OE_CHECK_MSG(impl != nullptr, "loadResourcesToContext: Invalid material context handle");

  createRootSignature(shaderResources, *impl);
  createMeshBufferViews(vsInputs, *impl);
}

void D3D12_material_manager::loadPipelineStateToContext(
        Material_context_handle materialContextHandle, const Pipeline_state_inputs& inputs)
{
  Material_context_impl* impl = getMaterialContextImpl(materialContextHandle);
  OE_CHECK_MSG(impl != nullptr, "loadPipelineStateToContext: Invalid material context handle");

  if (inputs.wireframe) {
    impl->pipelineStateDesc.rasterizerDesc = DirectX::CommonStates::Wireframe;
  }
  else if (impl->cullMode == Material_face_cull_mode::Back_face) {
    impl->pipelineStateDesc.rasterizerDesc = DirectX::CommonStates::CullClockwise;
  }
  else if (impl->cullMode == Material_face_cull_mode::Front_face) {
    impl->pipelineStateDesc.rasterizerDesc = DirectX::CommonStates::CullCounterClockwise;
  }
  else if (impl->cullMode == Material_face_cull_mode::None) {
    impl->pipelineStateDesc.rasterizerDesc = DirectX::CommonStates::CullNone;
  }
  else {
    OE_THROW(std::invalid_argument("Invalid cull mode"));
  }
  createPipelineState(*impl);

  impl->stateIdentifier.wireframeEnabled = inputs.wireframe;
}

bool D3D12_material_manager::isMaterialContextDataReady(Material_context_handle materialContext) const
{
  const Material_context_impl* impl = getMaterialContextImpl(materialContext);
  OE_CHECK_MSG(impl != nullptr, "isMaterialContextDataReady: Invalid material context handle");

  return impl->isDataReady;
}

void D3D12_material_manager::setDataReady(Material_context_handle materialContextHandle, bool ready)
{
  Material_context_impl* impl = getMaterialContextImpl(materialContextHandle);
  OE_CHECK_MSG(impl != nullptr, "setDataReady: Invalid material context handle");

  impl->isDataReady = ready;
}

void D3D12_material_manager::queueTextureLoad(const Texture& texture) { _textureManager.load(texture); }

void D3D12_material_manager::compileShader(
        const Material::Shader_compile_settings& compileSettings, bool enableOptimizations,
        const std::string& shaderTarget, ID3DBlob** result) const
{
  // Compiler defines
  std::vector<D3D_SHADER_MACRO> defines;
  for (const auto& define : compileSettings.defines) {
    defines.push_back({define.first.c_str(), define.second.c_str()});
  }
  // Null terminated array
  defines.push_back({nullptr, nullptr});

  // Enable better shader debugging with the graphics debugging tools.
  uint32_t compileFlags = enableOptimizations
                                  ? D3DCOMPILE_ENABLE_STRICTNESS
                                  : (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL);

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

  ///////////////////////////////
  /// Create a placeholder for constants at slot 0. This is populated by whomever calls DrawInstance
  if (impl.constantBufferCount) {
    d3dConstantDescriptorRanges[0] = {
            D3D12_DESCRIPTOR_RANGE_TYPE_CBV, impl.constantBufferCount, 0, 0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};
    // Don't know the address - just put a dummy value in here. It will be set after the fact

    LOG(DEBUG) << "  Root[" << rootParameters.size() << "] = CBV Table (size=" << impl.constantBufferCount << ")";

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
    for (const auto& textureResource : shaderResources.textures) {
      Texture* texture = textureResource.texture.get();
      OE_CHECK(texture->isValid());

      D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
      const bool isArray2dDesc = texture->isArray() || texture->isArraySlice();
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

      // DXGI_FORMAT
      switch (textureResource.format) {
        case Shader_texture_resource_format::Default:
          desc.Format = _textureManager.getFormat(*texture);
          break;
        case Shader_texture_resource_format::Depth_r24x8:
          desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
          break;
        case Shader_texture_resource_format::Stencil_x24u8:
          desc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
          break;
        default:
          OE_CHECK_FMT(
                  false, "Invalid resource format: %s",
                  shaderTextureResourceFormatToString(textureResource.format).c_str());
      }

      // PLANE SLICE
      if (desc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) {
        UINT& sliceField = isArray2dDesc ? desc.Texture2DArray.PlaneSlice : desc.Texture2D.PlaneSlice;
        sliceField = 1;
      }

      // VIEW DIMENSION / ARRAY SIZE
      ID3D12Resource* resource = _textureManager.getResource(*texture);
      auto resourceDesc = resource->GetDesc();
      OE_CHECK(resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

      if (texture->isCubeMap()) {
        desc.ViewDimension = isArray2dDesc ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;
      }
      else if (isArray2dDesc) {
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
      }
      else {
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      };

      // MIP SLICE / ARRAY SLICE
      if (isArray2dDesc) {
        if (texture->isArraySlice()) {
          desc.Texture2DArray.FirstArraySlice = texture->getArraySlice();
          desc.Texture2DArray.ArraySize = 1;
        }
        else {
          desc.Texture2DArray.FirstArraySlice = 0;
          desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
        }
        desc.Texture2DArray.MipLevels = resourceDesc.MipLevels;
        desc.Texture2DArray.MostDetailedMip = 0;
        desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
      }
      else {
        desc.Texture2D.MipLevels = resourceDesc.MipLevels;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.ResourceMinLODClamp = 0.0f;
      }

      _deviceResources.GetD3DDevice()->CreateShaderResourceView(resource, &desc, cpuHandle);
      cpuHandle.Offset(1, range.incrementSize);
    }

    d3dPsSrvDescriptorRanges[0] = {
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV, range.descriptorCount, 0, 0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};

    impl.rootSignatureRootDescriptorTables.emplace_back(range.gpuHandle);

    LOG(DEBUG) << "  Root[" << rootParameters.size() << "] = SRV Table (size=" << range.descriptorCount << ")";
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

    LOG(DEBUG) << "  Root[" << rootParameters.size() << "] = Sampler Table (size=" << numSamplerDescriptors << ")";
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

    if (impl.rootSignature.Get()) {
      _deviceResources.addReferenceToInFlightFrames(impl.rootSignature);
      impl.rootSignature = nullptr;
    }
    ThrowIfFailed(_deviceDependent.device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&impl.rootSignature)));

    std::wstring debugName = impl.name + L" (CompiledMaterial=" + impl.materialName + L")";
    SetObjectName(impl.rootSignature.Get(), debugName.c_str());
  }
}

void D3D12_material_manager::createMeshBufferViews(
        const std::vector<Vertex_attribute_element>& vsInputs, Material_context_impl& impl)
{
  // Define the vertex input layout.
  LOG(G3LOG_DEBUG) << "Adding vertex attributes";

  impl.inputElementDescs.clear();
  impl.inputElementDescs.reserve(vsInputs.size());
  for (auto inputSlot = 0U; inputSlot < vsInputs.size(); ++inputSlot) {
    const auto attrSemantic = vsInputs[inputSlot];
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
  if (impl.pixelShader) {
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(impl.pixelShader.Get());
  }
  psoDesc.PrimitiveTopologyType = impl.primitiveTopologyType;
  psoDesc.SampleMask = UINT_MAX;// TODO
  psoDesc.SampleDesc.Count = 1; // TODO - set quality and sample counts

  // Rasterizer state
  psoDesc.RasterizerState = impl.pipelineStateDesc.rasterizerDesc;
  psoDesc.BlendState = impl.blendDesc;
  psoDesc.DepthStencilState = impl.depthStencilDesc;
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

  if (impl.pipelineState) {
    _deviceResources.addReferenceToInFlightFrames(impl.pipelineState);
    impl.pipelineState.Reset();
  }
  ThrowIfFailed(_deviceDependent.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&impl.pipelineState)));
  LOG(INFO) << "Creating PSO for " << utf8_encode(impl.name);
  SetObjectName(impl.pipelineState.Get(), impl.name.c_str());
}

Material_context_impl* D3D12_material_manager::getMaterialContextImpl(Material_context_handle handle)
{
  if (handle <= 0) {
    return nullptr;
  }

  size_t index = materialContextHandleToIndex(handle);
  if (index >= _deviceDependent.materialContexts.size()) {
    return nullptr;
  }

  return _deviceDependent.materialContexts.at(index).get();
}

const Material_context_impl* D3D12_material_manager::getMaterialContextImpl(Material_context_handle handle) const
{
  if (handle <= 0) {
    return nullptr;
  }

  size_t index = materialContextHandleToIndex(handle);
  if (index >= _deviceDependent.materialContexts.size()) {
    return nullptr;
  }

  return _deviceDependent.materialContexts.at(index).get();
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

void D3D12_material_manager::bindMaterialContextToDevice(Material_context_handle materialContextHandle)
{
  Material_context_impl* impl = getMaterialContextImpl(materialContextHandle);
  OE_CHECK_MSG(impl != nullptr, "bindMaterialContextToDevice: Invalid material context handle");

  auto& frameResources = _deviceResources.getCurrentFrameResources();

  _deviceDependent.commandList->SetPipelineState(impl->pipelineState.Get());

  Root_signature_layout layout{
          impl->rootSignature.Get(), impl->constantBufferCount > 0 ? 0 : Root_signature_layout::kSlotInvalid};
  frameResources.setCurrentBoundRootSignature(layout);
  _deviceDependent.commandList->SetGraphicsRootSignature(impl->rootSignature.Get());

  auto constantBufferAllVisParamIndex = static_cast<uint32_t>(layout.constantBufferAllVisParamIndex);
  for (uint32_t index = 0; index < impl->rootSignatureRootDescriptorTables.size(); ++index) {
    if (index == constantBufferAllVisParamIndex) {
      continue;
    }
    _deviceDependent.commandList->SetGraphicsRootDescriptorTable(
            index, impl->rootSignatureRootDescriptorTables.at(index));
  }
}

void D3D12_material_manager::unbind()
{
  Material_manager::unbind();
  _deviceResources.getCurrentFrameResources().setCurrentBoundRootSignature({});
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
