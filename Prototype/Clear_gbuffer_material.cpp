#include "pch.h"
#include "Clear_gbuffer_material.h"

using namespace oe;
using namespace std::literals;

void Clear_gbuffer_material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const
{
	vertexAttributes.push_back(Vertex_attribute::Position);
}

uint32_t Clear_gbuffer_material::inputSlot(Vertex_attribute attribute)
{
	(void)attribute;
	return 0;
}

Material::Shader_compile_settings Clear_gbuffer_material::vertexShaderSettings() const
{
	auto settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/clear_gbuffer_VS.hlsl"s;
	return settings;
}

Material::Shader_compile_settings Clear_gbuffer_material::pixelShaderSettings() const
{
	auto settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/clear_gbuffer_PS.hlsl"s;
	return settings;
}
