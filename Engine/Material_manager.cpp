#include "pch.h"
#include "Material_manager.h"
#include "Material.h"
#include "Render_light_data.h"
#include "Renderer_data.h"
#include "Texture.h"
#include "Material_base.h"
#include "Scene.h"
#include "Material_context.h"

#include <d3dcompiler.h>
#include <locale>
#include <sstream>

#include <comdef.h>
#include "ID3D_resources_manager.h"
#include "Mesh_vertex_layout.h"
#include "Mesh_utils.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

const auto g_max_material_index = UINT8_MAX;
const std::string g_flag_disableOptimization = "disableOptimizations";

// Arbitrary limit
const auto g_max_shader_resource_count = 32;
std::vector<ID3D11ShaderResourceView*> g_nullShaderResourceViews;

const auto g_max_sampler_state_count = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
std::vector<ID3D11SamplerState*> g_nullSamplerStates;

std::string Material_manager::_name = "Material_manager";

template<>
IMaterial_manager* oe::create_manager(Scene & scene)
{
    return new Material_manager(scene);
}

Material_manager::Material_manager(Scene& scene)
    : IMaterial_manager(scene)
    , _materialConstants({})
    , _boundBlendMode(Render_pass_blend_mode::Opaque)
{
}

inline DX::DeviceResources& Material_manager::deviceResources() const
{
    return _scene.manager<ID3D_resources_manager>().deviceResources();
}

void Material_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{
    // Animation constant buffer
    std::array<Matrix, g_max_bone_transforms> initDataMem;
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
    DX::ThrowIfFailed(d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(bufferName.size()), bufferName.c_str()));

    _boneTransformConstantBuffer = std::make_unique<D3D_buffer>(d3dBuffer, dataSize);
}
void Material_manager::destroyDeviceDependentResources()
{
    
}

std::string createShaderError(HRESULT hr, ID3D10Blob* errorMessage, const Material::Shader_compile_settings& compileSettings)
{
    std::stringstream ss;

    const auto shaderFilenameUtf8 = utf8_encode(compileSettings.filename);
    ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

    // Get a pointer to the error message text buffer.
    if (errorMessage != nullptr) {
        const auto compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());
        ss << compileErrors << std::endl;
    }
    else
    {
        _com_error err(hr);
        ss << utf8_encode(std::wstring(err.ErrorMessage()));
    }

    return ss.str();
}

// todo: use this when creating a new material type...
/*
    if (idx > static_cast<size_t>(g_max_material_index)) {
        throw std::domain_error("Maximum number of unique materials exceeded");
    }
 */
void Material_manager::ensureMaterialConstants(const Material& material, Material_constants*& materialConstants)
{
    const auto idx = static_cast<size_t>(material.materialTypeIndex());
    if (idx >= _materialConstants.size()) {
        _materialConstants.resize(idx + 1);
    }
    materialConstants = &_materialConstants[idx];
}

void debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings)
{
    if (g3::logLevel(DEBUG)) {
        LOG(DEBUG) << prefix << " compile settings: " << nlohmann::json({
            { "defines", settings.defines },
            { "includes", settings.includes },
            { "filename", utf8_encode(settings.filename) },
            { "entryPoint", settings.entryPoint }
            }).dump(2);
    }
}

void Material_manager::createVertexShader(
    bool enableOptimizations,
    Material_context::Compiled_material& compiledMaterial,
    const Material& material) const
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
    ID3DBlob* vertexShaderByteCode;
    auto settings = material.vertexShaderSettings(compiledMaterial.flags);
    debugLogSettings("vertex shader", settings);

    std::vector<D3D_SHADER_MACRO> defines;
    for (const auto& define : settings.defines)
    {
        defines.push_back({
            define.first.c_str(),
            define.second.c_str()
            });
    }
    defines.push_back({ nullptr, nullptr });

    std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;

    LOG(G3LOG_DEBUG) << "Adding vertex attributes";

    for (auto inputSlot = 0u; inputSlot < compiledMaterial.vsInputs.size(); ++inputSlot)
    {
        const auto attrSemantic = compiledMaterial.vsInputs[inputSlot];
        const auto semanticName = Vertex_attribute_meta::semanticName(attrSemantic.semantic.attribute);
        assert(!semanticName.empty());
        LOG(G3LOG_DEBUG) << "  " << semanticName << " to slot " << inputSlot;
        inputElementDesc.push_back(
            {
                semanticName.data(),
                attrSemantic.semantic.semanticIndex,
                mesh_utils::getDxgiFormat(attrSemantic.type, attrSemantic.component),
                inputSlot,
                D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        );
    }

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    if (!enableOptimizations)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif

    hr = D3DCompileFromFile(settings.filename.c_str(),
        defines.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        settings.entryPoint.c_str(),
        "vs_5_0",
        flags, 0,
        &vertexShaderByteCode,
        errorMsgs.ReleaseAndGetAddressOf());
    if (!SUCCEEDED(hr)) {
        throw std::runtime_error(createShaderError(hr, errorMsgs.Get(), settings));
    }

    const auto device = deviceResources().GetD3DDevice();
    hr = device->CreateInputLayout(
        inputElementDesc.data(), 
        static_cast<UINT>(inputElementDesc.size()),
        vertexShaderByteCode->GetBufferPointer(), 
        vertexShaderByteCode->GetBufferSize(),
        &compiledMaterial.inputLayout);
    if (!SUCCEEDED(hr)) {
        throw std::runtime_error("Failed to create vertex input layout: "s + std::to_string(hr));
    }

    hr = device->CreateVertexShader(
        vertexShaderByteCode->GetBufferPointer(), 
        vertexShaderByteCode->GetBufferSize(), 
        nullptr, 
        &compiledMaterial.vertexShader);
    if (!SUCCEEDED(hr)) {
        throw std::runtime_error("Failed to vertex pixel shader: "s + std::to_string(hr));
    }
}

void Material_manager::createPixelShader(
    bool enableOptimizations,
    Material_context::Compiled_material& compiledMaterial,
    const Material& material) const
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
    ID3DBlob* pixelShaderByteCode;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    if (!enableOptimizations)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif
    
    auto settings = material.pixelShaderSettings(compiledMaterial.flags);
    debugLogSettings("pixel shader", settings);

    std::vector<D3D_SHADER_MACRO> defines;
    for (const auto& define : settings.defines)
    {
        defines.push_back({
            define.first.c_str(),
            define.second.c_str()
            });
    }
    defines.push_back({ nullptr, nullptr });

    hr = D3DCompileFromFile(settings.filename.c_str(),
        defines.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        settings.entryPoint.c_str(),
        "ps_5_0",
        flags, 0,
        &pixelShaderByteCode,
        errorMsgs.ReleaseAndGetAddressOf());
    if (!SUCCEEDED(hr)) {
        throw std::runtime_error(createShaderError(hr, errorMsgs.Get(), settings));
    }

    auto device = deviceResources().GetD3DDevice();
    hr = device->CreatePixelShader(
        pixelShaderByteCode->GetBufferPointer(),
        pixelShaderByteCode->GetBufferSize(),
        nullptr,
        &compiledMaterial.pixelShader);
    if (!SUCCEEDED(hr)) {
        throw std::runtime_error("Failed to create pixel shader: "s + std::to_string(hr));
    }

    auto debugName = "Material:" + utf8_encode(settings.filename);
    compiledMaterial.pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(debugName.size()), debugName.c_str());
}

void Material_manager::initialize()
{
    setRendererFeaturesEnabled(Renderer_features_enabled());
}

const std::string& Material_manager::name() const
{
    return _name;
}

void Material_manager::tick()
{
    if (_boundMaterial) {
        throw std::logic_error("Material is still bound after rendering complete! Did you forget to call IMaterial_manager::unbind() ?");
    }
}

void Material_manager::bind(
    Material_context& materialContext,
    std::shared_ptr<const Material> material,
    const Mesh_vertex_layout& meshVertexLayout,
    const Render_light_data& renderLightData,
    Render_pass_blend_mode blendMode,
    bool enablePixelShader)
{
    assert(!_boundMaterial);

    const auto materialHash = material->ensureCompilerPropertiesHash();
    auto compiledMaterial = materialContext.compiledMaterial;
    auto rebuildConfig = false;
    if (!compiledMaterial) {
        rebuildConfig = true;
    }
    else {
        if (compiledMaterial->blendMode != blendMode) {
            LOG(WARNING) << "Rendering material with a different blendMode than last time. This will be a big performance hit.";
            rebuildConfig = true;
        }
        else if (compiledMaterial->meshHash != meshVertexLayout.propertiesHash()) {
            LOG(WARNING) << "Rendering material with a different morphTargetCount than last time. This will be a big performance hit.";
            rebuildConfig = true;
        }
        else if (compiledMaterial->materialHash != materialHash) {
            LOG(DEBUG) << "Material hash changed.";
            rebuildConfig = true;
        }
        else if(compiledMaterial->rendererFeaturesHash != _rendererFeaturesHash) {
            LOG(DEBUG) << "Renderer features hash changed.";
            rebuildConfig = true;
        }
    }

    const auto device = deviceResources().GetD3DDevice();
    if (rebuildConfig)
    {
        auto flags = material->configFlags(_rendererFeatures, blendMode, meshVertexLayout);

        // Add flag for shader optimization, to determine if we need to recompile
        if (!_rendererFeatures.enableShaderOptimization)
            flags.insert(g_flag_disableOptimization);

        LOG(DEBUG) << "Material flags: " << nlohmann::json(flags).dump(2);

        auto requiresRecompile = true;
        if (compiledMaterial) {
            // Skip recompile if the flags are actually the same.
            if (flags.size() == compiledMaterial->flags.size() &&
                std::equal(flags.begin(), flags.end(), compiledMaterial->flags.begin())) 
            {
                requiresRecompile = false;
            }
        }
        else {
            compiledMaterial = std::make_shared<Material_context::Compiled_material>();
        }
        compiledMaterial->rendererFeaturesHash = _rendererFeaturesHash;

        if (requiresRecompile) {
            LOG(INFO) << "Recompiling shaders for material";

            // TODO: Look in a cache for a compiled material that matches the hash
            materialContext.compiledMaterial = compiledMaterial;

            compiledMaterial->materialHash = materialHash;
            compiledMaterial->meshHash = meshVertexLayout.propertiesHash();

            try {
                compiledMaterial->blendMode = blendMode;
                compiledMaterial->flags = std::move(flags);

                compiledMaterial->vsInputs = material->vertexInputs(compiledMaterial->flags);

                createVertexShader(_rendererFeatures.enableShaderOptimization, *compiledMaterial, *material);
                createPixelShader(_rendererFeatures.enableShaderOptimization, *compiledMaterial, *material);

                // Lazy create constant buffers for this material type if they don't already exist.
                Material_constants* materialConstants;
                ensureMaterialConstants(*material, materialConstants);

                if (!materialConstants->vertexConstantBuffer) {
                    materialConstants->vertexConstantBuffer = material->createVSConstantBuffer(device);
                }
                if (!materialConstants->pixelConstantBuffer) {
                    materialConstants->pixelConstantBuffer = material->createPSConstantBuffer(device);
                }

                // Make sure that the shader resource views and SamplerStates vectors are empty.
                materialContext.resetShaderResourceViews();
                materialContext.resetSamplerStates();
            }
            catch (std::exception& ex) {
                throw std::runtime_error("Failed to create resources in Material_manager::bind. "s + ex.what());
            }
        }

        const auto shaderResources = material->shaderResources(compiledMaterial->flags, renderLightData);
        if (materialContext.shaderResourceViews.size() < shaderResources.textures.size()) {
            materialContext.shaderResourceViews.resize(shaderResources.textures.size());

            for (auto idx = 0; idx < shaderResources.textures.size(); ++idx) {
                const auto& texture = shaderResources.textures[idx];
                if (!texture->isValid())
                    texture->load(device);
                assert(texture->isValid());

                const auto srv = texture->getShaderResourceView();
                if (materialContext.shaderResourceViews[idx] != srv) {
                    if (materialContext.shaderResourceViews[idx])
                        materialContext.shaderResourceViews[idx]->Release();

                    materialContext.shaderResourceViews[idx] = srv;
                    srv->AddRef();
                }
            }

            LOG(G3LOG_DEBUG) << "Created " << material->materialType() <<
                " shader resources. Texture Count: " << std::to_string(materialContext.shaderResourceViews.size());
        }

        // Only create sampler states if they don't exist.
        if (materialContext.samplerStates.size() < shaderResources.samplerDescriptors.size()) {
            materialContext.samplerStates.resize(shaderResources.samplerDescriptors.size());

            for (auto idx = 0; idx < shaderResources.samplerDescriptors.size(); ++idx) {
                const auto& descriptor = shaderResources.samplerDescriptors[idx];
                // TODO: A descriptor lookup?

                ThrowIfFailed(device->CreateSamplerState(&descriptor, &materialContext.samplerStates[idx]));
            }

            LOG(G3LOG_DEBUG) << "Created " << material->materialType() <<
                " sampler states. Count: " << std::to_string(materialContext.samplerStates.size());
        }
    }

    // We have a valid shader
    auto context = deviceResources().GetD3DDeviceContext();

    context->IASetInputLayout(compiledMaterial->inputLayout.Get());
    context->VSSetShader(compiledMaterial->vertexShader.Get(), nullptr, 0);

    if (enablePixelShader) {
        context->PSSetShader(compiledMaterial->pixelShader.Get(), nullptr, 0);

        // Set shader texture resource in the pixel shader.
        _boundSrvCount = static_cast<uint32_t>(materialContext.shaderResourceViews.size());
        if (_boundSrvCount > g_max_shader_resource_count) 
            throw std::runtime_error("Too many shaderResourceViews bound");
        context->PSSetShaderResources(0, _boundSrvCount, materialContext.shaderResourceViews.data());

        _boundSsCount = static_cast<uint32_t>(materialContext.samplerStates.size());
        if (_boundSsCount > g_max_sampler_state_count) 
            throw std::runtime_error("Too many samplerStates bound");
        context->PSSetSamplers(0, _boundSsCount, materialContext.samplerStates.data());
    }

    _boundMaterial = material;
    _boundBlendMode = blendMode;
    _boundLightDataConstantBuffer = renderLightData.buffer();
}

void Material_manager::render(
    const Renderer_data& rendererData,
    const Matrix& worldMatrix,
    const Renderer_animation_data& rendererAnimationData,
    const Render_pass::Camera_data& camera)
{
    assert(_boundMaterial);

    auto context = deviceResources().GetD3DDeviceContext();

    Material_constants* materialConstants;
    ensureMaterialConstants(*_boundMaterial, materialConstants);

    // Update constant buffers
    if (materialConstants->vertexConstantBuffer != nullptr) {

        _boundMaterial->updateVSConstantBuffer(worldMatrix, camera.viewMatrix, camera.projectionMatrix, rendererAnimationData, context, *materialConstants->vertexConstantBuffer);

        if (rendererAnimationData.numBoneTransforms) {
            assert(_boneTransformConstantBuffer &&
                rendererAnimationData.boneTransformConstants.size() <= _boneTransformConstantBuffer->size &&
                rendererAnimationData.numBoneTransforms < _boneTransformConstantBuffer->size);

            context->UpdateSubresource(
                _boneTransformConstantBuffer->d3dBuffer, 0, nullptr, rendererAnimationData.boneTransformConstants.data(), 0, 0
            );

            ID3D11Buffer* vertexConstantBuffers[] = { materialConstants->vertexConstantBuffer->d3dBuffer, _boneTransformConstantBuffer->d3dBuffer };
            context->VSSetConstantBuffers(0, 2, vertexConstantBuffers);
        }
        else {
            ID3D11Buffer* vertexConstantBuffers[] = { materialConstants->vertexConstantBuffer->d3dBuffer };
            context->VSSetConstantBuffers(0, 1, vertexConstantBuffers);            
        }

    }
    if (materialConstants->pixelConstantBuffer != nullptr) {
        _boundMaterial->updatePSConstantBuffer(worldMatrix, camera.viewMatrix, camera.projectionMatrix, context, *materialConstants->pixelConstantBuffer);
        ID3D11Buffer* pixelConstantBuffers[] = { materialConstants->pixelConstantBuffer->d3dBuffer, _boundLightDataConstantBuffer.Get() };
        context->PSSetConstantBuffers(0, 2, pixelConstantBuffers);
    }

    // Render the triangles
    if (rendererData.indexBufferAccessor != nullptr)
    {
        context->DrawIndexed(rendererData.indexCount, 0, 0);
    }
    else
    {
        context->Draw(rendererData.vertexCount, 0);
    }
}

void Material_manager::unbind()
{
    auto context = deviceResources().GetD3DDeviceContext();

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

    _boundMaterial.reset();
    _boundLightDataConstantBuffer.Reset();
}

void Material_manager::setRendererFeaturesEnabled(const Renderer_features_enabled& renderer_feature_enabled)
{
    _rendererFeatures = renderer_feature_enabled;
    _rendererFeaturesHash = _rendererFeatures.hash();
}

const Renderer_features_enabled& Material_manager::rendererFeatureEnabled() const
{
    return _rendererFeatures;
}

bool Material_manager::ensureSamplerState(
    Texture& texture,
    D3D11_TEXTURE_ADDRESS_MODE textureAddressMode,
    ID3D11SamplerState** d3D11SamplerState) const
{
    const auto device = deviceResources().GetD3DDevice();

    // TODO: Replace this method with that from PBRMaterial
    if (!texture.isValid())
    {
        try
        {
            texture.load(device);
        }
        catch (std::exception& e)
        {
            LOG(WARNING) << "Material: Failed to load texture: "s + e.what();
            return false;
        }
    }

    if (texture.isValid())
    {
        if (!*d3D11SamplerState) {
            D3D11_SAMPLER_DESC samplerDesc;
            // Create a texture sampler state description.
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = textureAddressMode;
            samplerDesc.AddressV = textureAddressMode;
            samplerDesc.AddressW = textureAddressMode;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            samplerDesc.BorderColor[0] = 0;
            samplerDesc.BorderColor[1] = 0;
            samplerDesc.BorderColor[2] = 0;
            samplerDesc.BorderColor[3] = 0;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            // Create the texture sampler state.
            ThrowIfFailed(device->CreateSamplerState(&samplerDesc, d3D11SamplerState));
        }
    }

    return true;
}
