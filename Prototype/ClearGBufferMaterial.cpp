#include "pch.h"
#include "ClearGBufferMaterial.h"

using namespace OE;
using namespace std::literals;

void ClearGBufferMaterial::getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const
{
	vertexAttributes.push_back(VertexAttribute::VA_POSITION);
}

UINT ClearGBufferMaterial::inputSlot(VertexAttribute attribute)
{
	(void)attribute;
	return 0;
}


Material::ShaderCompileSettings ClearGBufferMaterial::vertexShaderSettings() const
{
	ShaderCompileSettings settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/clear_gbuffer_VS.hlsl"s;
	return settings;
}

Material::ShaderCompileSettings ClearGBufferMaterial::pixelShaderSettings() const
{
	ShaderCompileSettings settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/clear_gbuffer_PS.hlsl"s;
	return settings;
}
