#include "pch.h"
#include "Unlit_material.h"

using namespace std::string_literals;
using namespace DirectX;
using namespace oe;

const std::string g_material_type = "Unlit_material";

Unlit_material::Unlit_material()
	: Material_base(Material_alpha_mode::Blend)
	, _baseColor(SimpleMath::Color(Colors::White))
{
}

const std::string& Unlit_material::materialType() const
{
	return g_material_type;
}

void Unlit_material::updateVSConstantBufferValues(Unlit_material_vs_constant_buffer& constants, 
	const SimpleMath::Matrix& /* worldMatrix */,
	const SimpleMath::Matrix& /* viewMatrix */,
	const SimpleMath::Matrix& /* projMatrix */)
{
	constants.baseColor = _baseColor;
}

void Unlit_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	assert(blendMode == Render_pass_blend_mode::Opaque);
}