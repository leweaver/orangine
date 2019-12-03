#include "pch.h"
#include "OeCore/Unlit_material.h"
#include "OeCore/JsonUtils.h"
#include "OeCore/Material_repository.h"

using namespace std::string_literals;
using namespace DirectX;
using namespace oe;

const std::string g_material_type = "Unlit_material";

const std::string g_json_baseColor = "base_color";

// Uses blended alpha, to ensure standard rendering pipeline (not deferred)
Unlit_material::Unlit_material()
	: Base_type(static_cast<uint8_t>(Material_type_index::Unlit), Material_alpha_mode::Blend)
	, _baseColor(Colors::White)
{
}

const std::string& Unlit_material::materialType() const
{
	return g_material_type;
}

nlohmann::json Unlit_material::serialize(bool compilerPropertiesOnly) const
{
    auto j = Base_type::serialize(compilerPropertiesOnly);

    if (!compilerPropertiesOnly) {
        j[g_json_baseColor] = _baseColor;
    }

    return j;
}

void Unlit_material::updateVSConstantBufferValues(Unlit_material_vs_constant_buffer& constants, 
	const SSE::Matrix4& /* worldMatrix */,
	const SSE::Matrix4& /* viewMatrix */,
	const SSE::Matrix4& /* projMatrix */,
    const Renderer_animation_data& /* rendererAnimationData */) const
{
	constants.baseColor = _baseColor;
}
