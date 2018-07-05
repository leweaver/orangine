#include "pch.h"
#include "Unlit_material.h"

using namespace std::string_literals;
using namespace DirectX;
using namespace oe;

Unlit_material::Unlit_material()
	: _baseColor(SimpleMath::Color(Colors::White))
{	
	setAlphaMode(Material_alpha_mode::Blend);
}

void Unlit_material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const
{
	vertexAttributes.push_back(Vertex_attribute::Position);
}

bool Unlit_material::createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Unlit_constants_vs);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	_constantsVs.worldViewProjection = SimpleMath::Matrix::Identity;
	_constantsVs.baseColor = _baseColor;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &_constantsVs;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

	std::string name("Unlit_material Constant Buffer");
	DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

	return true;
}

void Unlit_material::updateVSConstantBuffer(const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& viewMatrix,
	const SimpleMath::Matrix& projMatrix,
	ID3D11DeviceContext* context,
	ID3D11Buffer* buffer)
{
	// Convert to LH, for DirectX.
	_constantsVs.worldViewProjection = XMMatrixMultiplyTranspose(worldMatrix, XMMatrixMultiply(viewMatrix, projMatrix));

	_constantsVs.baseColor = _baseColor;

	context->UpdateSubresource(buffer, 0, nullptr, &_constantsVs, 0, 0);
}

void Unlit_material::createShaderResources(const DX::DeviceResources& deviceResources, Render_pass_output_format outputFormat)
{
	assert(outputFormat != Render_pass_output_format::Shaded_Unlit);
}

uint32_t Unlit_material::inputSlot(Vertex_attribute attribute)
{
	(void)attribute;
	return 0;
}

Material::Shader_compile_settings Unlit_material::vertexShaderSettings() const
{
	auto settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/unlit_VS.hlsl"s;
	return settings;
}

Material::Shader_compile_settings Unlit_material::pixelShaderSettings() const
{
	auto settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/vertex_colors_PS.hlsl"s;
	return settings;
}