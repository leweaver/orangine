﻿#include "pch.h"
#include "Deferred_light_material.h"
#include "Render_target_texture.h"

using namespace oe;
using namespace std::literals;
using namespace DirectX;
using namespace SimpleMath;

void Deferred_light_material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const
{
	vertexAttributes.push_back(Vertex_attribute::Position);
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

void Deferred_light_material::setupPointLight(const Vector3& lightPosition, const Color& color, float intensity)
{
	_constants.lightType = Deferred_light_type::Point;
	XMStoreFloat3(&_constants.position, XMVectorSet(lightPosition.x, lightPosition.y, lightPosition.z, 0.0f));

	XMVECTOR intensifiedColor = color;
	intensifiedColor = XMVectorScale(intensifiedColor, intensity);
	XMStoreFloat3(&_constants.intensifiedColor, intensifiedColor);
}

void Deferred_light_material::setupDirectionalLight(const Vector3& lightDirection, const Color& color, float intensity)
{
	_constants.lightType = Deferred_light_type::Directional;
	XMStoreFloat3(&_constants.direction, XMVectorSet(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f));

	XMVECTOR intensifiedColor = color;
	intensifiedColor = XMVectorScale(intensifiedColor, intensity);
	XMStoreFloat3(&_constants.intensifiedColor, intensifiedColor);
}

void Deferred_light_material::setupAmbientLight(const Color& color, float intensity)
{
	_constants.lightType = Deferred_light_type::Ambient;
	XMStoreFloat3(&_constants.position, XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));

	XMVECTOR intensifiedColor = color;
	intensifiedColor = XMVectorScale(intensifiedColor, intensity);
	XMStoreFloat3(&_constants.intensifiedColor, intensifiedColor);
}

void Deferred_light_material::setupEmitted()
{
	_constants.lightType = Deferred_light_type::Emitted;
}

bool Deferred_light_material::createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Deferred_light_constants);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	_constants.invProjection = Matrix::Identity;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &_constants;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	device->CreateBuffer(&bufferDesc, &initData, &buffer);

	std::string name("DeferredLightMaterial Constant Buffer");
	buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());

	return true;
}

void Deferred_light_material::updatePSConstantBuffer(const Matrix& worldMatrix,
	const Matrix& viewMatrix, 
	const Matrix& projMatrix, 
	ID3D11DeviceContext* context,
	ID3D11Buffer* buffer)
{
	// Convert to LH, for DirectX.
	XMVECTOR determinant;
	_constants.invProjection = XMMatrixTranspose(XMMatrixInverse(&determinant, projMatrix));
	_constants.eyePosition = Vector4(worldMatrix._41, worldMatrix._42, worldMatrix._43, 0.0);

	context->UpdateSubresource(buffer, 0, nullptr, &_constants, 0, 0);	
}

void Deferred_light_material::createBlendState(ID3D11Device* device, ID3D11BlendState*& blendState)
{
	// additive blend
	D3D11_BLEND_DESC blendStateDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendStateDesc.RenderTarget[0].BlendEnable = true;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	device->CreateBlendState(&blendStateDesc, &blendState);
}

void Deferred_light_material::setContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();
	if (!_color0Texture || !_color1Texture || !_color2Texture || !_depthTexture)
		return;
	
	auto validSamplers = true;
	if (!_color0SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color0Texture.get(), _color0SamplerState.ReleaseAndGetAddressOf());

	if (!_color1SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color1Texture.get(), _color1SamplerState.ReleaseAndGetAddressOf());

	if (!_color2SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_color2Texture.get(), _color2SamplerState.ReleaseAndGetAddressOf());

	if (!_depthSamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_depthTexture.get(), _depthSamplerState.ReleaseAndGetAddressOf());

	if (validSamplers)
	{
		ID3D11ShaderResourceView* shaderResourceViews[] = {
			_color0Texture->getShaderResourceView(),
			_color1Texture->getShaderResourceView(),
			_color2Texture->getShaderResourceView(),
			_depthTexture->getShaderResourceView()
		};
		// Set shader texture resource in the pixel shader.
		context->PSSetShaderResources(0, 4, shaderResourceViews);

		ID3D11SamplerState* samplerStates[] = {
			_color0SamplerState.Get(),
			_color1SamplerState.Get(),
			_color2SamplerState.Get(),
			_depthSamplerState.Get(),
		};
		context->PSSetSamplers(0, 4, samplerStates);
	}
}

void Deferred_light_material::unsetContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	ID3D11ShaderResourceView* shaderResourceViews[] = { nullptr, nullptr, nullptr, nullptr };
	context->PSSetShaderResources(0, 4, shaderResourceViews);

	ID3D11SamplerState* samplerStates[] = { nullptr, nullptr, nullptr, nullptr };
	context->PSSetSamplers(0, 4, samplerStates);
}