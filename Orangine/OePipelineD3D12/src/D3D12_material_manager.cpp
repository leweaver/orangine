#include "D3D12_material_manager.h"

#include <OeCore/Mesh_utils.h>

#include <d3dcompiler.h>
#include <cinttypes>
#include <comdef.h>

using oe::Material_context;
using oe::Renderer_data;
using Microsoft::WRL::ComPtr;

template<>
void oe::create_manager(
        Manager_instance<IMaterial_manager>& out, IAsset_manager& assetManager, ITexture_manager& textureManager,
        ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources)
{
  out = Manager_instance<IMaterial_manager>(std::make_unique<oe::pipeline_d3d12::D3D12_material_manager>(assetManager,
          textureManager, lightingManager, primitiveMeshDataFactory, deviceResources));
}

namespace oe::pipeline_d3d12 {
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

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;
  ComPtr<ID3D12PipelineState> pipelineState;
};

std::string createShaderErrorString(
        HRESULT hr,
        ID3D10Blob* errorMessage,
        const Material::Shader_compile_settings& compileSettings) {
  std::stringstream ss;

  const auto shaderFilenameUtf8 = compileSettings.filename;
  ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

  // Get a pointer to the error message text buffer.
  if (errorMessage != nullptr) {
    const auto compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());
    ss << compileErrors << std::endl;
  } else {
    const _com_error err(hr);
    ss << utf8_encode(std::wstring(err.ErrorMessage()));
  }

  return ss.str();
}

D3D12_material_manager::D3D12_material_manager(
        oe::IAsset_manager& assetManager, oe::ITexture_manager& textureManager, oe::ILighting_manager& lightingManager,
        oe::IPrimitive_mesh_data_factory& primitiveMeshDataFactory, oe::D3D12_device_resources& deviceResources)
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

  // Create an empty root signature.
  {
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(_deviceDependent.device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&_deviceDependent.rootSignature)));
  }
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

  auto impl = getMaterialContextImpl(materialContext);
  compileShader(compileSettings, impl.vertexShader.ReleaseAndGetAddressOf());
}

void D3D12_material_manager::createPixelShader(
        bool enableOptimizations, const Material& material, Material_context& materialContext) const
{
  // Generate a settings object from the given flags
  auto compileSettings = material.pixelShaderSettings(materialContext.compilerInputs.flags);
  debugLogSettings("pixel shader", compileSettings);

  auto impl = getMaterialContextImpl(materialContext);
  compileShader(compileSettings, impl.pixelShader.ReleaseAndGetAddressOf());
}

void D3D12_material_manager::compileShader(const Material::Shader_compile_settings& compileSettings, ID3DBlob** result) const
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
          utf8_decode(filename).c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, compileSettings.entryPoint.c_str(),
          "vs_5_0", compileFlags, 0, result, errorMsgs.ReleaseAndGetAddressOf());
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error(createShaderErrorString(hr, errorMsgs.Get(), compileSettings)));
  }
}

void D3D12_material_manager::loadShaderResourcesToContext(
        const oe::Material::Shader_resources& shaderResources, Material_context& materialContext)
{
  Material_context_impl& impl = getMaterialContextImpl(materialContext);

  // Define the vertex input layout.
  std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
  {
    LOG(G3LOG_DEBUG) << "Adding vertex attributes";

    inputElementDescs.reserve(materialContext.compilerInputs.vsInputs.size());
    for (auto inputSlot = 0U; inputSlot < materialContext.compilerInputs.vsInputs.size(); ++inputSlot) {
      const auto attrSemantic = materialContext.compilerInputs.vsInputs[inputSlot];
      const auto semanticName = Vertex_attribute_meta::semanticName(attrSemantic.semantic.attribute);
      OE_CHECK(!semanticName.empty());
      LOG(DEBUG) << "  " << semanticName << " to slot " << inputSlot;

      D3D12_INPUT_ELEMENT_DESC desc {
              semanticName.data(), attrSemantic.semantic.semanticIndex,
              mesh_utils::getDxgiFormat(attrSemantic.type, attrSemantic.component), inputSlot,
              D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
      inputElementDescs.push_back(desc);
    }
  }

  CD3DX12_BLEND_DESC blendDesc{D3D12_DEFAULT};

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = {inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size())};
  psoDesc.pRootSignature = _deviceDependent.rootSignature.Get();
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(impl.vertexShader.Get());
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(impl.pixelShader.Get());
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState = blendDesc;
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = getMeshDataTopology(impl.compilerInputs.meshIndexType);
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.SampleDesc.Count = 1;

  ThrowIfFailed(_deviceDependent.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&impl.pipelineState)));
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
Material_context_impl& D3D12_material_manager::getMaterialContextImpl(Material_context& context) const
{
  return static_cast<Material_context_impl&>(context);
}
}
