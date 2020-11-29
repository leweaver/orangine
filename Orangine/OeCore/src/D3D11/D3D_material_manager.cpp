#include "pch.h"

#include "D3D_material_manager.h"
#include "D3D_texture_manager.h"

#include "OeCore/EngineUtils.h"
#include "OeCore/ITexture_manager.h"
#include "OeCore/Material_base.h"
#include "OeCore/Mesh_utils.h"
#include "OeCore/Render_light_data.h"
#include "OeCore/Scene.h"

#include <comdef.h> // for _com_error
#include <d3d11.h>
#include <d3dcompiler.h>

using namespace oe;

// Arbitrary limit
const auto g_max_shader_resource_count = 32;
std::vector<ID3D11ShaderResourceView*> g_nullShaderResourceViews;

const auto g_max_sampler_state_count = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
std::vector<ID3D11SamplerState*> g_nullSamplerStates;

const size_t g_d3d_compiled_material_sentinel = 0xFFA500;

template <>
IMaterial_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<internal::D3D_device_repository>& deviceRepository) {
  return new D3D_material_manager(scene, deviceRepository);
}

void debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings) {
  if (g3::logLevel(DEBUG)) {
    LOG(DEBUG) << prefix << " compile settings: "
               << nlohmann::json({{"defines", settings.defines},
                                  {"includes", settings.includes},
                                  {"filename", utf8_encode(settings.filename)},
                                  {"entryPoint", settings.entryPoint}})
                      .dump(2);
  }
}

std::string createShaderError(
    HRESULT hr,
    ID3D10Blob* errorMessage,
    const Material::Shader_compile_settings& compileSettings) {
  std::stringstream ss;

  const auto shaderFilenameUtf8 = utf8_encode(compileSettings.filename);
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

D3D_material_manager::D3D_material_manager(
    Scene& scene,
    std::shared_ptr<internal::D3D_device_repository> deviceRepository)
    : Material_manager(scene), _materialConstants({}), _deviceRepository(deviceRepository) {

  _textureAddressModeLUT = {
      D3D11_TEXTURE_ADDRESS_WRAP,
      D3D11_TEXTURE_ADDRESS_MIRROR,
      D3D11_TEXTURE_ADDRESS_CLAMP,
      D3D11_TEXTURE_ADDRESS_BORDER,
      D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
  };
  _textureComparisonFuncLUT = {
      D3D11_COMPARISON_NEVER,
      D3D11_COMPARISON_LESS,
      D3D11_COMPARISON_EQUAL,
      D3D11_COMPARISON_LESS_EQUAL,
      D3D11_COMPARISON_GREATER,
      D3D11_COMPARISON_NOT_EQUAL,
      D3D11_COMPARISON_GREATER_EQUAL,
      D3D11_COMPARISON_ALWAYS,
  };

  // first part of the Sampler_filter_type enum name
  _textureMinFilterMinLUT = {
      D3D11_FILTER_TYPE_POINT,  // Point
      D3D11_FILTER_TYPE_LINEAR, // Linear
      D3D11_FILTER_TYPE_POINT,  // Point_mipmap_point
      D3D11_FILTER_TYPE_POINT,  // Point_mipmap_linear
      D3D11_FILTER_TYPE_LINEAR, // Linear_mipmap_point
      D3D11_FILTER_TYPE_LINEAR, // Linear_mipmap_linear
  };
  // second part of the Sampler_filter_type enum name
  _textureMinFilterMipLUT = {
      D3D11_FILTER_TYPE_POINT,  // Point
      D3D11_FILTER_TYPE_POINT,  // Linear
      D3D11_FILTER_TYPE_POINT,  // Point_mipmap_point
      D3D11_FILTER_TYPE_LINEAR, // Point_mipmap_linear
      D3D11_FILTER_TYPE_POINT,  // Linear_mipmap_point
      D3D11_FILTER_TYPE_LINEAR, // Linear_mipmap_linear
  };
  _textureMagFilterMagLUT = {
      D3D11_FILTER_TYPE_POINT,  // Point
      D3D11_FILTER_TYPE_LINEAR, // Linear
      D3D11_FILTER_TYPE_POINT,  // Point_mipmap_point
      D3D11_FILTER_TYPE_POINT,  // Point_mipmap_linear
      D3D11_FILTER_TYPE_POINT,  // Linear_mipmap_point
      D3D11_FILTER_TYPE_POINT,  // Linear_mipmap_linear
  };
}

inline DX::DeviceResources& D3D_material_manager::deviceResources() const {
  return _deviceRepository->deviceResources();
}

void D3D_material_manager::createDeviceDependentResources() {
  // Animation constant buffer
  std::array<SSE::Matrix4, g_max_bone_transforms> initDataMem;
  constexpr auto dataSize = initDataMem.size() * sizeof(decltype(initDataMem)::value_type);

  // Fill in a buffer description.
  D3D11_BUFFER_DESC bufferDesc;
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = static_cast<UINT>(dataSize);
  bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bufferDesc.CPUAccessFlags = 0;
  bufferDesc.MiscFlags = 0;

  // Fill in the sub-resource data.
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = initDataMem.data();
  initData.SysMemPitch = 0;
  initData.SysMemSlicePitch = 0;

  // Create the buffer
  ID3D11Buffer* d3dBuffer;
  ThrowIfFailed(deviceResources().GetD3DDevice()->CreateBuffer(&bufferDesc, &initData, &d3dBuffer));

  const std::string bufferName = "bone transforms CB";
  DX::ThrowIfFailed(d3dBuffer->SetPrivateData(
      WKPDID_D3DDebugObjectName, static_cast<UINT>(bufferName.size()), bufferName.c_str()));

  _boneTransformConstantBuffer = std::make_unique<D3D_buffer>(d3dBuffer, dataSize);
}

void D3D_material_manager::destroyDeviceDependentResources() {}

std::shared_ptr<D3D_buffer> D3D_material_manager::createVsConstantBuffer(
    const Material& material,
    ID3D11Device* device) {
  // If the constant buffer doesn't actually have any fields, don't bother creating one.
  if (material.isEmptyVertexShaderConstants()) {
    return nullptr;
  }

  D3D11_BUFFER_DESC bufferDesc;
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = material.vertexShaderConstantsSize();
  bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bufferDesc.CPUAccessFlags = 0;
  bufferDesc.MiscFlags = 0;

  if (bufferDesc.ByteWidth < _bufferTemp.size()) {
    _bufferTemp.resize(bufferDesc.ByteWidth);
  }
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = _bufferTemp.data();
  initData.SysMemPitch = 0;
  initData.SysMemSlicePitch = 0;

  ID3D11Buffer* buffer;
  DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

  auto name = material.materialType() + " Vertex CB";
  DX::ThrowIfFailed(buffer->SetPrivateData(
      WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

  return std::make_shared<D3D_buffer>(buffer, bufferDesc.ByteWidth);
}

std::shared_ptr<D3D_buffer> D3D_material_manager::createPsConstantBuffer(
    const Material& material,
    ID3D11Device* device) {
  // If the constant buffer doesn't actually have any fields, don't bother creating one.
  if (material.isEmptyPixelShaderConstants()) {
    return nullptr;
  }

  D3D11_BUFFER_DESC bufferDesc;
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = material.pixelShaderConstantsSize();
  bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bufferDesc.CPUAccessFlags = 0;
  bufferDesc.MiscFlags = 0;

  if (bufferDesc.ByteWidth < _bufferTemp.size()) {
    _bufferTemp.resize(bufferDesc.ByteWidth);
  }

  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = _bufferTemp.data();
  initData.SysMemPitch = 0;
  initData.SysMemSlicePitch = 0;

  ID3D11Buffer* buffer;
  DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

  auto name = material.materialType() + " Pixel CB";
  DX::ThrowIfFailed(buffer->SetPrivateData(
      WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

  return std::make_shared<D3D_buffer>(buffer, bufferDesc.ByteWidth);
}

void D3D_material_manager::updateVsConstantBuffer(
    const Material& material,
    const SSE::Matrix4& worldMatrix,
    const SSE::Matrix4& viewMatrix,
    const SSE::Matrix4& projMatrix,
    const Renderer_animation_data& rendererAnimationData,
    ID3D11DeviceContext* context,
    D3D_buffer& buffer) {
  assert(material.vertexShaderConstantsSize() <= _bufferTemp.size());

  material.updateVsConstantBuffer(
      worldMatrix,
      viewMatrix,
      projMatrix,
      rendererAnimationData,
      _bufferTemp.data(),
      material.vertexShaderConstantsSize());

  context->UpdateSubresource(buffer.d3dBuffer, 0, nullptr, _bufferTemp.data(), 0, 0);
}

void D3D_material_manager::updatePsConstantBuffer(
    const Material& material,
    const SSE::Matrix4& worldMatrix,
    const SSE::Matrix4& viewMatrix,
    const SSE::Matrix4& projMatrix,
    ID3D11DeviceContext* context,
    D3D_buffer& buffer) {

  assert(material.pixelShaderConstantsSize() <= _bufferTemp.size());
  material.updatePsConstantBuffer(
      worldMatrix, viewMatrix, projMatrix, _bufferTemp.data(), material.pixelShaderConstantsSize());

  context->UpdateSubresource(buffer.d3dBuffer, 0, nullptr, _bufferTemp.data(), 0, 0);
}

// todo: use this when creating a new material type...
/*
    if (idx > static_cast<size_t>(g_max_material_index)) {
        OE_THROW(std::domain_error("Maximum number of unique materials exceeded"));
    }
 */
void D3D_material_manager::ensureMaterialConstants(
    const Material& material,
    Material_constants*& materialConstants) {
  const auto idx = static_cast<size_t>(material.materialTypeIndex());
  if (idx >= _materialConstants.size()) {
    _materialConstants.resize(idx + 1);
  }
  materialConstants = &_materialConstants[idx];
}

D3D_material_context& D3D_material_manager::verifyAsD3dMaterialContext(
    Material_context& materialContext) {
  auto* cast = dynamic_cast<D3D_material_context*>(&materialContext);
  assert(cast != nullptr);
  return *cast;
}

const D3D_material_context& D3D_material_manager::verifyAsD3dMaterialContext(
    const Material_context& materialContext) {
  const auto* const cast = dynamic_cast<const D3D_material_context*>(&materialContext);
  assert(cast != nullptr);
  return *cast;
}

const D3D_compiled_material& D3D_material_manager::verifyAsD3dCompiledMaterial(
    const Material_context::Compiled_material* compiledMaterial) {
  const auto* const cast = reinterpret_cast<const D3D_compiled_material*>(compiledMaterial);
  assert(cast->sentinel == g_d3d_compiled_material_sentinel);
  return *cast;
}

D3D_compiled_material& D3D_material_manager::verifyAsD3dCompiledMaterial(
    Material_context::Compiled_material* compiledMaterial) {
  auto* const cast = reinterpret_cast<D3D_compiled_material*>(compiledMaterial);
  assert(cast->sentinel == g_d3d_compiled_material_sentinel);
  return *cast;
}

std::unique_ptr<Material_context> D3D_material_manager::createMaterialContext() {
  return std::make_unique<D3D_material_context>();
}

Material_context::Compiled_material* D3D_material_manager::createCompiledMaterialData() const {
  auto* cm = new D3D_compiled_material();
  cm->sentinel = g_d3d_compiled_material_sentinel;
  return &cm->header;
}

void D3D_material_manager::createVertexShader(
    bool enableOptimizations,
    Material_context::Compiled_material* compiledMaterial,
    const Material& material) const {
  HRESULT hr;
  Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
  ID3DBlob* vertexShaderByteCode;
  auto settings = material.vertexShaderSettings(compiledMaterial->flags);
  debugLogSettings("vertex shader", settings);

  std::vector<D3D_SHADER_MACRO> defines;
  for (const auto& define : settings.defines) {
    defines.push_back({define.first.c_str(), define.second.c_str()});
  }
  defines.push_back({nullptr, nullptr});

  std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;

  LOG(G3LOG_DEBUG) << "Adding vertex attributes";

  for (auto inputSlot = 0u; inputSlot < compiledMaterial->vsInputs.size(); ++inputSlot) {
    const auto attrSemantic = compiledMaterial->vsInputs[inputSlot];
    const auto semanticName = Vertex_attribute_meta::semanticName(attrSemantic.semantic.attribute);
    assert(!semanticName.empty());
    LOG(G3LOG_DEBUG) << "  " << semanticName << " to slot " << inputSlot;
    inputElementDesc.push_back(
        {semanticName.data(),
         attrSemantic.semantic.semanticIndex,
         mesh_utils::getDxgiFormat(attrSemantic.type, attrSemantic.component),
         inputSlot,
         D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA,
         0});
  }

  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
  if (!enableOptimizations)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif

  const auto filename = shaderPath() + L"/" + settings.filename;
  LOG(DEBUG) << "Compiling shader " << filename.c_str();
  hr = D3DCompileFromFile(
      filename.c_str(),
      defines.data(),
      D3D_COMPILE_STANDARD_FILE_INCLUDE,
      settings.entryPoint.c_str(),
      "vs_5_0",
      flags,
      0,
      &vertexShaderByteCode,
      errorMsgs.ReleaseAndGetAddressOf());
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error(createShaderError(hr, errorMsgs.Get(), settings)));
  }

  auto& d3dCompiledMaterial = verifyAsD3dCompiledMaterial(compiledMaterial);

  const auto device = deviceResources().GetD3DDevice();
  hr = device->CreateInputLayout(
      inputElementDesc.data(),
      static_cast<UINT>(inputElementDesc.size()),
      vertexShaderByteCode->GetBufferPointer(),
      vertexShaderByteCode->GetBufferSize(),
      &d3dCompiledMaterial.inputLayout);
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error("Failed to create vertex input layout: " + std::to_string(hr)));
  }

  hr = device->CreateVertexShader(
      vertexShaderByteCode->GetBufferPointer(),
      vertexShaderByteCode->GetBufferSize(),
      nullptr,
      &d3dCompiledMaterial.vertexShader);
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error("Failed to vertex pixel shader: " + std::to_string(hr)));
  }
}

void D3D_material_manager::createPixelShader(
    bool enableOptimizations,
    Material_context::Compiled_material* compiledMaterial,
    const Material& material) const {
  HRESULT hr;
  Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
  ID3DBlob* pixelShaderByteCode;

  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
  if (!enableOptimizations)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif

  auto settings = material.pixelShaderSettings(compiledMaterial->flags);
  debugLogSettings("pixel shader", settings);

  std::vector<D3D_SHADER_MACRO> defines;
  for (const auto& define : settings.defines) {
    defines.push_back({define.first.c_str(), define.second.c_str()});
  }
  defines.push_back({nullptr, nullptr});

  const auto filename = shaderPath() + L"/" + settings.filename;
  LOG(DEBUG) << "Compiling shader " << filename.c_str();
  hr = D3DCompileFromFile(
      filename.c_str(),
      defines.data(),
      D3D_COMPILE_STANDARD_FILE_INCLUDE,
      settings.entryPoint.c_str(),
      "ps_5_0",
      flags,
      0,
      &pixelShaderByteCode,
      errorMsgs.ReleaseAndGetAddressOf());
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error(createShaderError(hr, errorMsgs.Get(), settings)));
  }

  auto& d3dCompiledMaterial = verifyAsD3dCompiledMaterial(compiledMaterial);

  auto device = deviceResources().GetD3DDevice();
  hr = device->CreatePixelShader(
      pixelShaderByteCode->GetBufferPointer(),
      pixelShaderByteCode->GetBufferSize(),
      nullptr,
      &d3dCompiledMaterial.pixelShader);
  if (!SUCCEEDED(hr)) {
    OE_THROW(std::runtime_error("Failed to create pixel shader: " + std::to_string(hr)));
  }

  auto debugName = "Material:" + utf8_encode(settings.filename);
  d3dCompiledMaterial.pixelShader->SetPrivateData(
      WKPDID_D3DDebugObjectName, static_cast<UINT>(debugName.size()), debugName.c_str());
}

void D3D_material_manager::createMaterialConstants(const Material& material) {
  // Lazy create constant buffers for this material type if they don't already exist.
  Material_constants* materialConstants;
  ensureMaterialConstants(material, materialConstants);

  auto* const device = deviceResources().GetD3DDevice();

  if (!materialConstants->vertexConstantBuffer) {
    materialConstants->vertexConstantBuffer = createVsConstantBuffer(material, device);
  }
  if (!materialConstants->pixelConstantBuffer) {
    materialConstants->pixelConstantBuffer = createPsConstantBuffer(material, device);
  }
}

D3D11_FILTER D3D_material_manager::convertFilter(
    const Sampler_descriptor& descriptor,
    float maxAnisotropy) {
  auto comparison = descriptor.comparisonFunc != Sampler_comparison_func::Never;

  if (maxAnisotropy > 1.0f) {
    return D3D11_ENCODE_ANISOTROPIC_FILTER(static_cast<D3D11_COMPARISON_FUNC>(comparison));
  }
  D3D11_FILTER_TYPE dxMin = _textureMinFilterMinLUT[(int)descriptor.minFilter];
  D3D11_FILTER_TYPE dxMip = _textureMinFilterMipLUT[(int)descriptor.minFilter];
  D3D11_FILTER_TYPE dxMag = _textureMagFilterMagLUT[(int)descriptor.magFilter];

  return D3D11_ENCODE_BASIC_FILTER(
      dxMin, dxMag, dxMip, static_cast<D3D11_FILTER_REDUCTION_TYPE>(comparison));
}

void D3D_material_manager::loadShaderResourcesToContext(
    const Material::Shader_resources& shaderResources,
    Material_context& materialContext) {

  auto& d3dMaterialContext = verifyAsD3dMaterialContext(materialContext);

  if (d3dMaterialContext.shaderResourceViews.size() < shaderResources.textures.size()) {
    d3dMaterialContext.shaderResourceViews.resize(shaderResources.textures.size());

    for (size_t idx = 0; idx < shaderResources.textures.size(); ++idx) {
      const auto& texturePtr = shaderResources.textures[idx];
      auto& d3dTexture = D3D_texture_manager::verifyAsD3dTexture(*texturePtr);

      if (!d3dTexture.isValid()) {
        _scene.manager<ITexture_manager>().load(d3dTexture);
      }

      assert(d3dTexture.isValid());

      const auto srv = d3dTexture.getShaderResourceView();
      if (d3dMaterialContext.shaderResourceViews[idx] != srv) {
        if (d3dMaterialContext.shaderResourceViews[idx])
          d3dMaterialContext.shaderResourceViews[idx]->Release();

        d3dMaterialContext.shaderResourceViews[idx] = srv;
        srv->AddRef();
      }
    }

    LOG(G3LOG_DEBUG) << "Created shader resources. Texture Count: "
                     << std::to_string(d3dMaterialContext.shaderResourceViews.size());
  }

  // Only create sampler states if they don't exist.
  if (d3dMaterialContext.samplerStates.size() < shaderResources.samplerDescriptors.size()) {
    d3dMaterialContext.samplerStates.resize(shaderResources.samplerDescriptors.size());

    auto* const device = deviceResources().GetD3DDevice();
    for (size_t idx = 0; idx < shaderResources.samplerDescriptors.size(); ++idx) {
      const auto& descriptor = shaderResources.samplerDescriptors[idx];

      // TODO: A cache of sampler states so we don't need to create a million of them?

      auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
      samplerDesc.AddressU = _textureAddressModeLUT[(int)descriptor.wrapU];
      samplerDesc.AddressV = _textureAddressModeLUT[(int)descriptor.wrapV];
      samplerDesc.AddressW = _textureAddressModeLUT[(int)descriptor.wrapW];

      samplerDesc.ComparisonFunc = _textureComparisonFuncLUT[(int)descriptor.comparisonFunc];

      // TODO: Experiment with anisotropic filtering.
      samplerDesc.Filter = convertFilter(descriptor, 0.0f);

      ThrowIfFailed(
          device->CreateSamplerState(&samplerDesc, &d3dMaterialContext.samplerStates[idx]));
    }

    LOG(G3LOG_DEBUG) << "Created sampler states. Count: "
                     << std::to_string(d3dMaterialContext.samplerStates.size());
  }
}

void D3D_material_manager::bindLightDataToDevice(const Render_light_data* renderLightData) {
  _boundLightDataConstantBuffer = renderLightData->buffer();
}

void D3D_material_manager::bindMaterialContextToDevice(
    const Material_context& materialContext,
    bool enablePixelShader) {
  const auto& d3dMaterialContext = verifyAsD3dMaterialContext(materialContext);
  auto& d3dCompiledMaterial =
      verifyAsD3dCompiledMaterial(d3dMaterialContext.compiledMaterial.get());

  auto* const deviceContext = deviceResources().GetD3DDeviceContext();

  deviceContext->IASetInputLayout(d3dCompiledMaterial.inputLayout.Get());
  deviceContext->VSSetShader(d3dCompiledMaterial.vertexShader.Get(), nullptr, 0);

  if (enablePixelShader) {
    deviceContext->PSSetShader(d3dCompiledMaterial.pixelShader.Get(), nullptr, 0);

    // Set shader texture resource in the pixel shader.
    _boundSrvCount = static_cast<uint32_t>(d3dMaterialContext.shaderResourceViews.size());
    if (_boundSrvCount > g_max_shader_resource_count)
      OE_THROW(std::runtime_error("Too many shaderResourceViews bound"));
    deviceContext->PSSetShaderResources(
        0, _boundSrvCount, d3dMaterialContext.shaderResourceViews.data());

    _boundSsCount = static_cast<uint32_t>(d3dMaterialContext.samplerStates.size());
    if (_boundSsCount > g_max_sampler_state_count)
      OE_THROW(std::runtime_error("Too many samplerStates bound"));
    deviceContext->PSSetSamplers(0, _boundSsCount, d3dMaterialContext.samplerStates.data());
  }
}

void D3D_material_manager::render(
    const Renderer_data& rendererData,
    const SSE::Matrix4& worldMatrix,
    const Renderer_animation_data& rendererAnimationData,
    const Render_pass::Camera_data& camera) {
  assert(getBoundMaterial() != nullptr);
  const auto& boundMaterial = *getBoundMaterial();

  ID3D11DeviceContext1* context = deviceResources().GetD3DDeviceContext();

  Material_constants* materialConstants;
  ensureMaterialConstants(boundMaterial, materialConstants);

  // Update constant buffers
  if (materialConstants->vertexConstantBuffer != nullptr) {
    updateVsConstantBuffer(
        boundMaterial,
        worldMatrix,
        camera.viewMatrix,
        camera.projectionMatrix,
        rendererAnimationData,
        context,
        *materialConstants->vertexConstantBuffer);

    if (rendererAnimationData.numBoneTransforms) {
      assert(
          _boneTransformConstantBuffer &&
          rendererAnimationData.boneTransformConstants.size() <=
              _boneTransformConstantBuffer->size &&
          rendererAnimationData.numBoneTransforms < _boneTransformConstantBuffer->size);

      context->UpdateSubresource(
          _boneTransformConstantBuffer->d3dBuffer,
          0,
          nullptr,
          rendererAnimationData.boneTransformConstants.data(),
          0,
          0);

      ID3D11Buffer* vertexConstantBuffers[] = {
          materialConstants->vertexConstantBuffer->d3dBuffer,
          _boneTransformConstantBuffer->d3dBuffer};
      context->VSSetConstantBuffers(0, 2, vertexConstantBuffers);
    } else {
      ID3D11Buffer* vertexConstantBuffers[] = {materialConstants->vertexConstantBuffer->d3dBuffer};
      context->VSSetConstantBuffers(0, 1, vertexConstantBuffers);
    }
  }
  if (materialConstants->pixelConstantBuffer != nullptr) {
    updatePsConstantBuffer(
        boundMaterial,
        worldMatrix,
        camera.viewMatrix,
        camera.projectionMatrix,
        context,
        *materialConstants->pixelConstantBuffer);
    ID3D11Buffer* pixelConstantBuffers[] = {
        materialConstants->pixelConstantBuffer->d3dBuffer, _boundLightDataConstantBuffer.Get()};
    context->PSSetConstantBuffers(0, 2, pixelConstantBuffers);
  }

  // Render the triangles
  if (rendererData.indexBufferAccessor != nullptr) {
    context->DrawIndexed(rendererData.indexCount, 0, 0);
  } else {
    context->Draw(rendererData.vertexCount, 0);
  }
}

void D3D_material_manager::unbind() {
  Material_manager::unbind();

  auto* context = deviceResources().GetD3DDeviceContext();

  if (_boundSrvCount) {
    if (_boundSrvCount > g_nullShaderResourceViews.size()) {
      g_nullShaderResourceViews.resize(_boundSrvCount);
    }
    context->PSSetShaderResources(0, _boundSrvCount, g_nullShaderResourceViews.data());
  }
  if (_boundSsCount) {
    if (_boundSsCount > g_nullSamplerStates.size()) {
      g_nullSamplerStates.resize(_boundSsCount);
    }
    context->PSSetSamplers(0, _boundSsCount, g_nullSamplerStates.data());
  }

  context->IASetInputLayout(nullptr);
  context->VSSetShader(nullptr, nullptr, 0);
  context->PSSetShader(nullptr, nullptr, 0);

  _boundLightDataConstantBuffer.Reset();
}