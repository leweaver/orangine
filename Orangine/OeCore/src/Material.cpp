#include "OeCore/Material.h"

#include "OeCore/Renderer_data.h"
#include "OeCore/Texture.h"
#include "OeCore/Mesh_vertex_layout.h"

using namespace oe;
using namespace std::string_literals;

const std::string g_json_alphaMode = "alpha_mode";
const std::string g_json_faceCullMode = "face_cull_mode";
const std::string g_json_enabled = "enabled";
const std::string g_json_texture = "texture";

Material::Material(uint8_t materialTypeIndex, Material_alpha_mode alphaMode, Material_face_cull_mode faceCullMode)
	: _requiresRecompile(true)
    , _materialTypeIndex(materialTypeIndex)
	, _alphaMode(alphaMode)
	, _faceCullMode(faceCullMode)
{
}

Material::Shader_resources Material::shaderResources(const std::set<std::string>& flags, const Render_light_data& renderLightData) const
{
    return {};
}

std::set<std::string> Material::configFlags(
    const Renderer_features_enabled& rendererFeatures, 
    Render_pass_blend_mode blendMode, 
    const Mesh_vertex_layout& meshBindContext
) const
{
    return {};
}

Material::Shader_compile_settings Material::vertexShaderSettings(const std::set<std::string>& flags) const
{
	return Shader_compile_settings
	{
		L"data/shaders/vertex_colors_VS.hlsl"s,
		"VSMain"s,
		std::map<std::string, std::string>(),
		std::set<std::string>()
	};
}

Material::Shader_compile_settings Material::pixelShaderSettings(const std::set<std::string>& flags) const
{
	return Shader_compile_settings
	{
		L"data/shaders/vertex_colors_PS.hlsl"s,
		"PSMain"s,
		std::map<std::string, std::string>(),
		std::set<std::string>()
	};
}

nlohmann::json Material::serialize(bool /* compilerPropertiesOnly */) const
{
    return {
        { g_json_alphaMode, materialAlphaModeToString(_alphaMode) },
        { g_json_faceCullMode, materialFaceCullModeToString(_faceCullMode) }
    };
}

nlohmann::json Material::serializeTexture(bool compilerPropertiesOnly, const std::shared_ptr<Texture>& texture)
{
    auto j = nlohmann::json{{ g_json_enabled, texture != nullptr }};
    if (!compilerPropertiesOnly) {
        j[g_json_texture] = texture;
    }
    return j;
}

size_t Material::calculateCompilerPropertiesHash()
{
    if (_requiresRecompile) {
        const auto j = serialize(true).dump();
        if (g3::logLevel(DEBUG)) {
            LOG(DEBUG) << "Serialized compiler affecting material properties:\n" << j;
        }
        _propertiesHash = std::hash<std::string>{}(j);
        _requiresRecompile = false;
    }
    return _propertiesHash;
}

size_t Material::ensureCompilerPropertiesHash() const
{
    if (_requiresRecompile)
        OE_THROW(std::domain_error("Material requires recompile"));
    return _propertiesHash;
}
