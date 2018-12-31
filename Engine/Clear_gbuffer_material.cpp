#include "pch.h"
#include "Clear_gbuffer_material.h"
#include "Render_pass_config.h"

using namespace oe;
using namespace std::literals;

const std::string g_material_type = "Clear_gbuffer_material";

const std::string& Clear_gbuffer_material::materialType() const
{
	return g_material_type;
}

void Clear_gbuffer_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	assert(blendMode == Render_pass_blend_mode::Opaque);
}
