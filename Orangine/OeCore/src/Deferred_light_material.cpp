#include "pch.h"
#include "OeCore/Deferred_light_material.h"
#include "OeCore/Render_light_data.h"
#include "OeCore/Material_repository.h"

using namespace oe;
using namespace std::literals;
using namespace DirectX;

const auto g_mrt_shader_resource_count = 9;
const auto g_mrt_sampler_state_count = 6;

const std::string g_json_color0Texture = "color0_texture";
const std::string g_json_color1Texture = "color1_texture";
const std::string g_json_color2Texture = "color2_texture";
const std::string g_json_depthTexture = "depth_texture";
const std::string g_json_shadowMapDepthTexture = "shadow_map_depth_texture";
const std::string g_json_shadowMapStencilTexture = "shadow_map_stencil_texture";
const std::string g_json_iblEnabled = "ibl_enabled";
const std::string g_json_shadowArrayEnabled = "shadow_array_enabled";
const std::string g_json_shadowMapCount = "shadow_map_count";
const std::string g_json_emittedEnabled = "emitted_enabled";

const std::string g_material_type = "Deferred_light_material";

const std::string g_flag_debugNormals = "debug_normals";
const std::string g_flag_shadowsEnabled = "shadowsEnabled";
const std::string g_flag_iblEnabled = "iblEnabled";

Deferred_light_material::Deferred_light_material()
    : Base_type(static_cast<uint8_t>(Material_type_index::Deferred_Light))
{}

const std::string& Deferred_light_material::materialType() const
{
	return g_material_type;
}

nlohmann::json Deferred_light_material::serialize(bool compilerPropertiesOnly) const
{
    auto j = Base_type::serialize(compilerPropertiesOnly);

    // TODO: Store asset ID's here.
    if (!compilerPropertiesOnly) {
        j[g_json_shadowMapCount] = _shadowMapCount;
        j[g_json_emittedEnabled] = _emittedEnabled;
    }

    j[g_json_color0Texture] = serializeTexture(compilerPropertiesOnly, _color0Texture);
    j[g_json_color1Texture] = serializeTexture(compilerPropertiesOnly, _color1Texture);
    j[g_json_color2Texture] = serializeTexture(compilerPropertiesOnly, _color2Texture);
    j[g_json_depthTexture] = serializeTexture(compilerPropertiesOnly, _depthTexture);
    j[g_json_shadowMapDepthTexture] = serializeTexture(compilerPropertiesOnly, _shadowMapDepthTexture);
    j[g_json_shadowMapStencilTexture] = serializeTexture(compilerPropertiesOnly, _shadowMapStencilTexture);
    j[g_json_iblEnabled] = _iblEnabled;
    j[g_json_shadowArrayEnabled] = _shadowArrayEnabled;

    return j;
}

std::set<std::string> Deferred_light_material::configFlags(const Renderer_features_enabled& rendererFeatures,
    Render_pass_blend_mode blendMode, const Mesh_vertex_layout& meshBindContext) const
{
    auto flags = Base_type::configFlags(rendererFeatures, blendMode, meshBindContext);

    if (rendererFeatures.debugDisplayMode == Debug_display_mode::Normals) {
        flags.insert(g_flag_debugNormals);
    }
    if (rendererFeatures.shadowsEnabled && _shadowArrayEnabled) {
        flags.insert(g_flag_shadowsEnabled);
    }
    if (rendererFeatures.irradianceMappingEnabled && _iblEnabled) {
        flags.insert(g_flag_iblEnabled);
    }

    return flags;
}

Material::Shader_resources Deferred_light_material::shaderResources(const std::set<std::string>& flags,
    const Render_light_data& renderLightData) const
{
    auto sr = Base_type::shaderResources(flags, renderLightData);

    if (!_color0Texture || !_color1Texture || !_color2Texture || !_depthTexture)
        return sr;

    auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    // TODO: is the comparison function even used?
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

    sr.textures.push_back(_color0Texture);
    sr.samplerDescriptors.push_back(samplerDesc);

    sr.textures.push_back(_color1Texture);
    sr.samplerDescriptors.push_back(samplerDesc);

    sr.textures.push_back(_color2Texture);
    sr.samplerDescriptors.push_back(samplerDesc);

    sr.textures.push_back(_depthTexture);
    sr.samplerDescriptors.push_back(samplerDesc);

    if (flags.find(g_flag_iblEnabled) != flags.end()) {
        if (!(renderLightData.environmentMapBrdf() && renderLightData.environmentMapDiffuse() && renderLightData.environmentMapSpecular())) {
            throw std::runtime_error("Deferred_light_material requires a valid environment map be provided in the Render_light_data");
        }

        sr.textures.push_back(renderLightData.environmentMapBrdf());
        sr.textures.push_back(renderLightData.environmentMapDiffuse());
        sr.textures.push_back(renderLightData.environmentMapSpecular());

        // Create IBL sampler desc
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sr.samplerDescriptors.push_back(samplerDesc);
    }

    if (flags.find(g_flag_shadowsEnabled) != flags.end()) {
        if (!(_shadowMapStencilTexture && _shadowMapDepthTexture)) {
            throw std::logic_error("Cannot bind a shadow map stencil texture without a shadow map depth texture.");
        }

        sr.textures.push_back(_shadowMapDepthTexture);
        sr.textures.push_back(_shadowMapStencilTexture);

        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sr.samplerDescriptors.push_back(samplerDesc);
    }

    return sr;
}

Material::Shader_compile_settings Deferred_light_material::pixelShaderSettings(const std::set<std::string>& flags) const
{
    auto settings = Base_type::pixelShaderSettings(flags);

    if (flags.find(g_flag_iblEnabled) != flags.end()) {
        settings.defines["MAP_IBL"] = "1";
    }

    if (flags.find(g_flag_shadowsEnabled) != flags.end()) {
        settings.defines["MAP_SHADOWMAP_ARRAY"] = "1";
    }

    if (flags.find(g_flag_debugNormals) != flags.end()) {
        settings.defines["DEBUG_DISPLAY_NORMALS"] = "1";
    }

	//settings.defines["DEBUG_DISPLAY_METALLIC_ROUGHNESS"] = "1";
	//settings.defines["DEBUG_LIGHTING_ONLY"] = "1";
	//settings.defines["DEBUG_NO_LIGHTING"] = "1";
	return settings;
}

void Deferred_light_material::setupEmitted(bool enabled)
{
	_emittedEnabled = enabled;
}

void Deferred_light_material::updatePSConstantBufferValues(Deferred_light_material_constant_buffer& constants,
	const SSE::Matrix4& worldMatrix,
	const SSE::Matrix4& viewMatrix,
	const SSE::Matrix4& projMatrix) const
{
	// Convert to LH, for DirectX. 
	constants.viewMatrixInv = SSE::inverse(viewMatrix);
	constants.projMatrixInv = SSE::inverse(projMatrix);
	/*
	LOG(DEBUG) <<
		"\n{" <<
		"\n  " << worldMatrix._11 << ", " << worldMatrix._12 << ", " << worldMatrix._13 << ", " << worldMatrix._14 <<
		"\n  " << worldMatrix._21 << ", " << worldMatrix._22 << ", " << worldMatrix._23 << ", " << worldMatrix._24 <<
		"\n  " << worldMatrix._31 << ", " << worldMatrix._32 << ", " << worldMatrix._33 << ", " << worldMatrix._34 <<
		"\n  " << worldMatrix._41 << ", " << worldMatrix._42 << ", " << worldMatrix._43 << ", " << worldMatrix._44 <<
		"\n}";
		*/

	// Inverse of the view matrix is the camera transform matrix
	// TODO: This is redundant, remove from the constants.
	constants.eyePosition = { constants.viewMatrixInv.getTranslation(), 0.0 };
	constants.emittedEnabled = _emittedEnabled;
}