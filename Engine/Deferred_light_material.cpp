#include "pch.h"
#include "Deferred_light_material.h"
#include "Render_pass_info.h"
#include "Render_light_data.h"

#include <array>

using namespace oe;
using namespace std::literals;
using namespace DirectX;
using namespace SimpleMath;

const auto g_mrt_sampler_count = 5;

const std::array<ID3D11ShaderResourceView*, g_mrt_sampler_count> g_nullShaderResourceViews = { nullptr, nullptr, nullptr, nullptr, nullptr };
const std::array<ID3D11SamplerState*, g_mrt_sampler_count> g_nullSamplerStates = { nullptr, nullptr, nullptr, nullptr, nullptr };

void Deferred_light_material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const
{
	vertexAttributes.push_back(Vertex_attribute::Position);
}

void Deferred_light_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	assert(blendMode == Render_pass_blend_mode::Additive);
}

UINT Deferred_light_material::inputSlot(Vertex_attribute attribute)
{
	(void)attribute;
	return 0;
}

Material::Shader_compile_settings Deferred_light_material::vertexShaderSettings() const
{
	Shader_compile_settings settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/deferred_light_VS.hlsl"s;
	return settings;
}

Material::Shader_compile_settings Deferred_light_material::pixelShaderSettings() const
{
	Shader_compile_settings settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/deferred_light_PS.hlsl"s;
	//settings.defines["DEBUG_DISPLAY_METALLIC_ROUGHNESS"] = "1";
	//settings.defines["DEBUG_LIGHTING_ONLY"] = "1";
	//settings.defines["DEBUG_NO_LIGHTING"] = "1";
	//settings.defines["DEBUG_DISPLAY_NORMALS"] = "1";
	return settings;
}

void Deferred_light_material::setupEmitted(bool enabled)
{
	_constants.emittedEnabled = enabled;
}

bool Deferred_light_material::createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Deferred_light_material_constants);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	_constants.viewMatrixInv = Matrix::Identity;
	_constants.projMatrixInv = Matrix::Identity;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &_constants;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	device->CreateBuffer(&bufferDesc, &initData, &buffer);

	std::string name("DeferredLightMaterial Constant Buffer");
	buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());

	return true;
}

void Deferred_light_material::updatePSConstantBuffer(const Render_light_data& renderlightData, 
	const Matrix& worldMatrix,
	const Matrix& viewMatrix, 
	const Matrix& projMatrix, 
	ID3D11DeviceContext* context,
	ID3D11Buffer* buffer)
{
	const auto viewMatrixInv = viewMatrix.Invert();

	// Convert to LH, for DirectX. 
	_constants.viewMatrixInv = XMMatrixTranspose(viewMatrixInv);
	_constants.projMatrixInv = XMMatrixTranspose(XMMatrixInverse(nullptr, projMatrix));
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
	_constants.eyePosition = Vector4(viewMatrixInv._41, viewMatrixInv._42, viewMatrixInv._43, 0.0);

	context->UpdateSubresource(buffer, 0, nullptr, &_constants, 0, 0);
}

void Deferred_light_material::setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData)
{
	auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();
	if (!_color0Texture || !_color1Texture || !_color2Texture || !_depthTexture)
		return;
	
	auto validSamplers = true;
	if (!_color0SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color0Texture.get(), D3D11_TEXTURE_ADDRESS_WRAP, &_color0SamplerState);

	if (!_color1SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color1Texture.get(), D3D11_TEXTURE_ADDRESS_WRAP, &_color1SamplerState);

	if (!_color2SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color2Texture.get(), D3D11_TEXTURE_ADDRESS_WRAP, &_color2SamplerState);

	if (!_depthSamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_depthTexture.get(), D3D11_TEXTURE_ADDRESS_WRAP, &_depthSamplerState);
	
	if (_shadowMapTexture && !_shadowMapSamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_shadowMapTexture.get(), D3D11_TEXTURE_ADDRESS_CLAMP, &_shadowMapSamplerState);

	if (validSamplers)
	{
		std::array<ID3D11ShaderResourceView*, g_mrt_sampler_count> shaderResourceViews = {
			_color0Texture->getShaderResourceView(),
			_color1Texture->getShaderResourceView(),
			_color2Texture->getShaderResourceView(),
			_depthTexture->getShaderResourceView(),
			_shadowMapTexture->getShaderResourceView()
		};

		// Set shader texture resource in the pixel shader.
		context->PSSetShaderResources(0, g_mrt_sampler_count, shaderResourceViews.data());

		std::array<ID3D11SamplerState*, g_mrt_sampler_count> samplerStates = {
			_color0SamplerState.Get(),
			_color1SamplerState.Get(),
			_color2SamplerState.Get(),
			_depthSamplerState.Get(),
			_shadowMapSamplerState.Get()
		};
		context->PSSetSamplers(0, g_mrt_sampler_count, samplerStates.data());
	}
}

void Deferred_light_material::unsetContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	context->PSSetShaderResources(0, g_mrt_sampler_count, g_nullShaderResourceViews.data());
	context->PSSetSamplers(0, g_mrt_sampler_count, g_nullSamplerStates.data());
}