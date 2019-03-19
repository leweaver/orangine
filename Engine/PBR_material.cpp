﻿#include "pch.h"
#include "PBR_material.h"
#include "Render_pass_config.h"

#include "JsonUtils.h"

#include "Material_repository.h"
#include "Mesh_vertex_layout.h"

using namespace oe;
using namespace DirectX;
using namespace std::literals;

const std::array<ID3D11ShaderResourceView*, 5> g_nullShaderResourceViews = { nullptr, nullptr, nullptr, nullptr, nullptr };
const std::array<ID3D11SamplerState*, 5> g_nullSamplerStates = { nullptr, nullptr, nullptr, nullptr, nullptr };

const std::string g_material_type = "PBR_material";
const std::string g_json_baseColor = "baseColor";
const std::string g_json_metallic = "metallic";
const std::string g_json_roughness = "roughness";
const std::string g_json_emissive = "emissive";
const std::string g_json_alphaCutoff = "alpha_cutoff";
const std::string g_json_baseColorTexture = "base_color_texture";
const std::string g_json_metallicRoughnessTexture = "metallic_roughness_texture";
const std::string g_json_normalTexture = "normal_texture";
const std::string g_json_occlusionTexture = "occlusion_texture";
const std::string g_json_emissiveTexture = "emissive_texture";

const std::string g_flag_enableDeferred = "enable_deferred";
const std::string g_flag_morphTargetCount_prefix = "morph_target_count_";
// Position in the layout of each morph attribute
const std::string g_flag_morphTargetLayout_Position_prefix = "morph_target_vertex_position_";
const std::string g_flag_morphTargetLayout_Normal_prefix = "morph_target_vertex_normal_";
const std::string g_flag_morphTargetLayout_Tangent_prefix = "morph_target_vertex_tangent_";
const std::string g_flag_skinned = "skinned";
const std::string g_flag_skinned_joints_uint16 = "joints_uint16";
const std::string g_flag_skinned_joints_uint32 = "joints_uint32";
const std::string g_flag_skinned_joints_sint16 = "joints_sint16";
const std::string g_flag_skinned_joints_sint32 = "joints_sint32";

PBR_material::PBR_material()
	: Base_type(static_cast<uint8_t>(Material_type_index::PBR))
    , _baseColor(SimpleMath::Vector4::One)
	, _metallic(1.0)
	, _roughness(1.0)
	, _emissive(0, 0, 0)
	, _alphaCutoff(0.5)
	, _boundTextureCount(0)
{
	std::fill(_textures.begin(), _textures.end(), nullptr);
	std::fill(_shaderResourceViews.begin(), _shaderResourceViews.end(), nullptr);
	std::fill(_samplerStates.begin(), _samplerStates.end(), nullptr);
}

const std::string& PBR_material::materialType() const
{
	return g_material_type;
}

nlohmann::json PBR_material::serialize(bool compilerPropertiesOnly) const
{
    auto j = Base_type::serialize(compilerPropertiesOnly);

    if (!compilerPropertiesOnly) {
        j[g_json_baseColor] = _baseColor;
        j[g_json_metallic] = _metallic;
        j[g_json_roughness] = _roughness;
        j[g_json_emissive] = _emissive;
        j[g_json_alphaCutoff] = _alphaCutoff;
        j[g_json_baseColorTexture] = serializeTexture(compilerPropertiesOnly, _textures[BaseColor]);
        j[g_json_metallicRoughnessTexture] = serializeTexture(compilerPropertiesOnly, _textures[MetallicRoughness]);
        j[g_json_normalTexture] = serializeTexture(compilerPropertiesOnly, _textures[Normal]);
        j[g_json_occlusionTexture] = serializeTexture(compilerPropertiesOnly, _textures[Occlusion]);
        j[g_json_emissiveTexture] = serializeTexture(compilerPropertiesOnly, _textures[Emissive]);
    }

    return j;
}

std::set<std::string> PBR_material::configFlags(
    const Renderer_features_enabled& rendererFeatures,
    Render_pass_blend_mode blendMode, 
    const Mesh_vertex_layout& meshVertexLayout) const
{
    auto flags = Base_type::configFlags(rendererFeatures, blendMode, meshVertexLayout);

    // TODO: This is a hacky way of finding this information out.
    if (blendMode == Render_pass_blend_mode::Opaque) {
        flags.insert(g_flag_enableDeferred);
    }

    const auto vertexLayout = meshVertexLayout.vertexLayout();
    const auto hasJoints = std::any_of(
        vertexLayout.begin(),
        vertexLayout.end(),
        [](const auto& vae) { return vae.semantic == Vertex_attribute_semantic{ Vertex_attribute::Joints, 0 }; });
    const auto hasWeights = std::any_of(
        vertexLayout.begin(),
        vertexLayout.end(),
        [](const auto& vae) { return vae.semantic == Vertex_attribute_semantic{ Vertex_attribute::Weights, 0 }; });

    if (rendererFeatures.skinnedAnimation && hasJoints && hasWeights) {
        flags.insert(g_flag_skinned);

        for (const auto mve : meshVertexLayout.vertexLayout()) {
            if (mve.semantic == Vertex_attribute_semantic{ Vertex_attribute::Joints, 0 }) {
                if (mve.component == Element_component::Unsigned_Short)
                    flags.insert(g_flag_skinned_joints_uint16);
                else if (mve.component == Element_component::Unsigned_Int)
                    flags.insert(g_flag_skinned_joints_uint32);
                else if (mve.component == Element_component::Signed_Short)
                    flags.insert(g_flag_skinned_joints_sint16);
                else if (mve.component == Element_component::Signed_Int)
                    flags.insert(g_flag_skinned_joints_sint32);
                else
                    throw std::runtime_error("Material does not support joints component: " + elementComponentToString(mve.component));
            }
        }
    }

    if (rendererFeatures.vertexMorph && meshVertexLayout.morphTargetCount()) {
        flags.insert(g_flag_morphTargetCount_prefix + std::to_string(meshVertexLayout.morphTargetCount()));

        const auto& morphTargetLayout = meshVertexLayout.morphTargetLayout();
        for (auto i = 0; i < morphTargetLayout.size(); ++i) {
            if (morphTargetLayout[i] == Vertex_attribute_semantic{ Vertex_attribute::Position, 0 }) {
                flags.insert(g_flag_morphTargetLayout_Position_prefix + std::to_string(i));
            } 
            else if (morphTargetLayout[i] == Vertex_attribute_semantic{ Vertex_attribute::Normal, 0 }) {
                flags.insert(g_flag_morphTargetLayout_Normal_prefix + std::to_string(i));
            }
            else if (morphTargetLayout[i] == Vertex_attribute_semantic{ Vertex_attribute::Tangent, 0 }) {
                flags.insert(g_flag_morphTargetLayout_Tangent_prefix + std::to_string(i));
            }
        }
    }

    return flags;
}

void PBR_material::decodeMorphTargetConfig(const std::set<std::string>& flags,
    uint8_t& targetCount,
    int8_t& positionPosition,
    int8_t& normalPosition,
    int8_t& tangentPosition)
{
    targetCount = 0;
    positionPosition = -1;
    normalPosition = -1;
    tangentPosition = -1;
    for (const auto& flag : flags) {
        uint16_t convert;
        if (str_starts(flag, g_flag_morphTargetCount_prefix)) {
            std::stringstream ss(flag.substr(g_flag_morphTargetCount_prefix.size()));
            ss >> convert;
            assert(convert >= 0 && convert <= 255);
            targetCount = static_cast<uint8_t>(convert);
        }
        else if (str_starts(flag, g_flag_morphTargetLayout_Position_prefix)) {
            std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Position_prefix.size()));
            ss >> convert;
            assert(convert >= 0 && convert <= 255);
            positionPosition = static_cast<int8_t>(convert);
        }
        else if (str_starts(flag, g_flag_morphTargetLayout_Normal_prefix)) {
            std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Normal_prefix.size()));
            ss >> convert;
            assert(convert >= 0 && convert <= 255);
            normalPosition = static_cast<int8_t>(convert);
        }
        else if (str_starts(flag, g_flag_morphTargetLayout_Tangent_prefix)) {
            std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Tangent_prefix.size()));
            ss >> convert;
            assert(convert >= 0 && convert <= 255);
            tangentPosition = static_cast<int8_t>(convert);
        }
    }
}

int PBR_material::getMorphPositionAttributeIndexOffset() { return 1; }

int PBR_material::getMorphNormalAttributeIndexOffset() { return 1; }

int PBR_material::getMorphTangentAttributeIndexOffset() const { return requiresTangents() ? 1 : 0; }

std::vector<Vertex_attribute_element> PBR_material::vertexInputs(const std::set<std::string>& flags) const
{
    auto vertexAttributes = Base_type::vertexInputs(flags);

    vertexAttributes.push_back(Vertex_attribute_element{ { Vertex_attribute::Normal, 0 }, Element_type::Vector3, Element_component::Float });

    if (requiresTangents()) {
        vertexAttributes.push_back({ { Vertex_attribute::Tangent, 0 }, Element_type::Vector4, Element_component::Float });
    }

    if (requiresTexCoord0()) {
        vertexAttributes.push_back({{ Vertex_attribute::Tex_Coord, 0 }, Element_type::Vector2, Element_component::Float });
    }

    if (flags.find(g_flag_skinned) != flags.end()) {
        Element_component jointsComponent;
        if (flags.find(g_flag_skinned_joints_uint16) != flags.end())
            jointsComponent = Element_component::Unsigned_Short;
        else if (flags.find(g_flag_skinned_joints_uint32) != flags.end())
            jointsComponent = Element_component::Unsigned_Int;
        else if (flags.find(g_flag_skinned_joints_sint16) != flags.end())
            jointsComponent = Element_component::Signed_Short;
        else if (flags.find(g_flag_skinned_joints_sint32) != flags.end())
            jointsComponent = Element_component::Signed_Int;
        else
            throw std::runtime_error("Missing joints component flag");

        vertexAttributes.push_back({ { Vertex_attribute::Joints, 0 }, Element_type::Vector4, jointsComponent });
        vertexAttributes.push_back({{ Vertex_attribute::Weights, 0 }, Element_type::Vector4, Element_component::Float });
    }

    // Vertex attributes for morph targets.
    uint8_t targetCount;
    int8_t positionPosition;
    int8_t normalPosition;
    int8_t tangentPosition;
    decodeMorphTargetConfig(flags, targetCount, positionPosition, normalPosition, tangentPosition);

    if (targetCount > 8)
        throw std::domain_error("Does not support more than 8 morph targets");

    uint8_t morphPositionSemanticOffset = getMorphPositionAttributeIndexOffset();
    uint8_t morphNormalSemanticOffset = getMorphNormalAttributeIndexOffset();
    uint8_t morphTangentSemanticOffset = getMorphTangentAttributeIndexOffset();
    for (auto i = 0; i < targetCount; ++i) {
        if (positionPosition >= 0) {
            vertexAttributes.push_back({ Vertex_attribute::Position, morphPositionSemanticOffset++ });
        }
        if (normalPosition >= 0) {
            vertexAttributes.push_back({ Vertex_attribute::Normal, morphNormalSemanticOffset++ });
        }
        if (requiresTangents() && tangentPosition >= 0) {
            vertexAttributes.push_back({ Vertex_attribute::Tangent, morphTangentSemanticOffset++ });
        }
    }

    return vertexAttributes;
}

Material::Shader_resources PBR_material::shaderResources(const Render_light_data& renderLightData) const
{
    auto sr = Base_type::shaderResources(renderLightData);

    const auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
    for (auto i = 0; i < _textures.size(); ++i) {
        const auto& texture = _textures[i];
        if (!texture)
            continue;

        sr.textures.push_back(texture);
        
        // TODO: Use values from glTF
        sr.samplerDescriptors.push_back(samplerDesc);
    }

    return sr;
}

Material::Shader_compile_settings PBR_material::vertexShaderSettings(const std::set<std::string>& flags) const
{
    auto settings = Base_type::vertexShaderSettings(flags);
    applyVertexLayoutShaderCompileSettings(settings);

    // Skinning
    if (flags.find(g_flag_skinned) != flags.end()) {
        settings.defines["VB_SKINNED"] = "1";
    }
    
    uint8_t targetCount;
    int8_t positionPosition;
    int8_t normalPosition;
    int8_t tangentPosition;
    decodeMorphTargetConfig(flags, targetCount, positionPosition, normalPosition, tangentPosition);

    // Add morph inputs to the vertex input struct
    if (targetCount > 0) {
        uint8_t morphPositionSemanticOffset = getMorphPositionAttributeIndexOffset();
        uint8_t morphNormalSemanticOffset = getMorphNormalAttributeIndexOffset();
        uint8_t morphTangentSemanticOffset = getMorphTangentAttributeIndexOffset();

        std::array<std::stringstream, 3> vbMorphWeightsCalcParts;
        vbMorphWeightsCalcParts[0] << "float4 morphedPosition = float4(skinnedPosition.xyz";
        vbMorphWeightsCalcParts[1] << "float3 morphedNormal = float3(Input.vNormal";
        vbMorphWeightsCalcParts[2] << "float4 morphedTangent = float4(Input.vTangent.xyz";

        // Build inputs.
        std::array<std::stringstream, 3> vbMorphInputsArr;

        for (auto morphTargetIdx = 0; morphTargetIdx < targetCount; ++morphTargetIdx) {
            const auto morphTargetIdxStr = std::to_string(static_cast<int16_t>(morphTargetIdx));
            if (positionPosition != -1) {
                const auto semanticIdx = static_cast<uint8_t>(morphPositionSemanticOffset++);
                const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
                vbMorphInputsArr.at(positionPosition) <<
                    "float3 vMorphPosition" << morphTargetIdxStr <<
                    " : POSITION" << semanticIdxStr <<
                    ";";
                vbMorphWeightsCalcParts[0] << " + g_morphWeights" << (morphTargetIdx / 4) << "[" << (morphTargetIdx % 4) << "]" << " * Input.vMorphPosition" << morphTargetIdxStr;
                settings.morphAttributes.push_back(
                    { Vertex_attribute::Position, semanticIdx }
                );
            }
            if (normalPosition != -1) {
                const auto semanticIdx = static_cast<uint8_t>(morphNormalSemanticOffset++);
                const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
                vbMorphInputsArr.at(normalPosition) <<
                    "float3 vMorphNormal" << morphTargetIdxStr <<
                    " : NORMAL" << semanticIdxStr <<
                    ";";
                vbMorphWeightsCalcParts[1] << " + g_morphWeights" << (morphTargetIdx / 4) << "[" << (morphTargetIdx % 4) << "]" << " * Input.vMorphNormal" << morphTargetIdxStr;
                settings.morphAttributes.push_back(
                    { Vertex_attribute::Normal, semanticIdx }
                );
            }
            if (requiresTangents() && tangentPosition != -1) {
                const auto semanticIdx = static_cast<uint8_t>(morphTangentSemanticOffset++);
                const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
                vbMorphInputsArr.at(tangentPosition) <<
                    "float3 vMorphTangent" << morphTargetIdxStr <<
                    " : TANGENT" << semanticIdxStr <<
                    ";";
                vbMorphWeightsCalcParts[2] << " + g_morphWeights" << (morphTargetIdx / 4) << "[" << (morphTargetIdx % 4) << "]" << " * Input.vMorphTangent" << morphTargetIdxStr;
                settings.morphAttributes.push_back(
                    { Vertex_attribute::Tangent, semanticIdx }
                );
            }
        }

        vbMorphWeightsCalcParts[0] << ", skinnedPosition.w);";
        vbMorphWeightsCalcParts[1] << ");";
        vbMorphWeightsCalcParts[2] << ", Input.vTangent.w); ";

        std::stringstream vbMorphWeightsCalc;
        vbMorphWeightsCalc << vbMorphWeightsCalcParts[0].str();
        vbMorphWeightsCalc << vbMorphWeightsCalcParts[1].str();

        if (requiresTangents())
            vbMorphWeightsCalc << vbMorphWeightsCalcParts[2].str();

        settings.defines["VB_MORPH"] = "1";
        settings.defines["VB_MORPH_INPUTS"] = vbMorphInputsArr[0].str() + 
            vbMorphInputsArr[1].str() +
            vbMorphInputsArr[2].str();
        settings.defines["VB_MORPH_WEIGHTS_CALC"] = vbMorphWeightsCalc.str();
    }

    return settings;
}

Material::Shader_compile_settings PBR_material::pixelShaderSettings(const std::set<std::string>& flags) const
{
	auto settings = Base_type::pixelShaderSettings(flags);

    if (_textures[BaseColor])
        settings.defines["MAP_BASECOLOR"] = "1";
    if (_textures[MetallicRoughness])
        settings.defines["MAP_METALLIC_ROUGHNESS"] = "1";
    if (_textures[Normal])
        settings.defines["MAP_NORMAL"] = "1";
    if (_textures[Emissive])
        settings.defines["MAP_EMISSIVE"] = "1";
    if (_textures[Occlusion])
        settings.defines["MAP_OCCLUSION"] = "1";

	if (flags.find(g_flag_enableDeferred) != flags.end())
		settings.defines["PS_PIPELINE_DEFERRED"] = "1";
	else
		settings.defines["PS_PIPELINE_STANDARD"] = "1";

	if (getAlphaMode() == Material_alpha_mode::Mask)
		settings.defines["ALPHA_MASK_VALUE"] = std::to_string(alphaCutoff());

    applyVertexLayoutShaderCompileSettings(settings);

	return settings;
}

void PBR_material::applyVertexLayoutShaderCompileSettings(Shader_compile_settings& settings) const
{
    settings.defines["VB_NORMAL"] = "1";
    if (requiresTexCoord0()) {
        settings.defines["VB_TEXCOORD0"] = "1";
    }
    if (requiresTangents()) {
        settings.defines["VB_TANGENT"] = "1";
    }
}

void PBR_material::updateVSConstantBufferValues(PBR_material_vs_constant_buffer& constants,
	const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& viewMatrix,
	const SimpleMath::Matrix& projMatrix,
    const Renderer_animation_data& rendererAnimationData) const
{
	// Note that HLSL matrices are Column Major (as opposed to Row Major in DirectXMath) - so we need to transpose everything.
    constants.viewProjection = XMMatrixMultiplyTranspose(viewMatrix, projMatrix);
	constants.world = XMMatrixTranspose(worldMatrix);
    constants.worldInvTranspose = XMMatrixInverse(nullptr, worldMatrix);
    memcpy_s(&constants.morphWeights[0].x,
        sizeof(XMFLOAT4) * array_size(constants.morphWeights),
        rendererAnimationData.morphWeights.data(),
        sizeof(decltype(rendererAnimationData.morphWeights)::value_type) * rendererAnimationData.morphWeights.size()
    );
}

void PBR_material::updatePSConstantBufferValues(PBR_material_ps_constant_buffer& constants,
	const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& /* viewMatrix */,
	const SimpleMath::Matrix& /* projMatrix */) const
{
	// Convert to LH, for DirectX.
	constants.world = XMMatrixTranspose(worldMatrix);
	constants.baseColor = _baseColor;
	constants.metallicRoughness = SimpleMath::Vector4(_metallic, _roughness, 0.0, 0.0);
	constants.emissive = SimpleMath::Vector4(_emissive.x, _emissive.y, _emissive.z, 0.0);
	constants.eyePosition = SimpleMath::Vector4(worldMatrix._41, worldMatrix._42, worldMatrix._43, 0.0);
}
