#include "pch.h"
#include "DeferredLightMaterial.h"
#include "TextureRenderTarget.h"

using namespace OE;
using namespace std::literals;

void DeferredLightMaterial::getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const
{
	vertexAttributes.push_back(VertexAttribute::VA_POSITION);
}

UINT DeferredLightMaterial::inputSlot(VertexAttribute attribute)
{
	(void)attribute;
	return 0;
}


Material::ShaderCompileSettings DeferredLightMaterial::vertexShaderSettings() const
{
	ShaderCompileSettings settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/deferred_light_VS.hlsl"s;
	return settings;
}

Material::ShaderCompileSettings DeferredLightMaterial::pixelShaderSettings() const
{
	ShaderCompileSettings settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/deferred_light_PS.hlsl"s;
	return settings;
}

bool DeferredLightMaterial::createConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer)
{
	return true;
}

void DeferredLightMaterial::updateConstantBuffer(const DirectX::XMMATRIX &worldMatrix,
	const DirectX::XMMATRIX &viewMatrix, const DirectX::XMMATRIX &projMatrix, ID3D11DeviceContext *context,
	ID3D11Buffer *buffer)
{
}

void DeferredLightMaterial::setContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();
	if (!m_color0Texture || !m_color1Texture || !m_depthTexture)
		return;
	
	bool validSamplers = true;
	if (!m_color0SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *m_color0Texture.get(), m_color0SamplerState.ReleaseAndGetAddressOf());

	if (!m_color1SamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *m_color1Texture.get(), m_color1SamplerState.ReleaseAndGetAddressOf());

	if (!m_depthSamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *m_depthTexture.get(), m_depthSamplerState.ReleaseAndGetAddressOf());

	if (validSamplers)
	{
		ID3D11ShaderResourceView *shaderResourceViews[3] = {
			m_color0Texture->getShaderResourceView(),
			m_color1Texture->getShaderResourceView(),
			m_depthTexture->getShaderResourceView()
		};
		// Set shader texture resource in the pixel shader.
		context->PSSetShaderResources(0, 3, shaderResourceViews);

		ID3D11SamplerState *samplerStates[3] = {
			m_color0SamplerState.Get(),
			m_color1SamplerState.Get(),
			m_depthSamplerState.Get(),
		};
		context->PSSetSamplers(0, 3, samplerStates);
	}
}

void DeferredLightMaterial::unsetContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	ID3D11ShaderResourceView *shaderResourceViews[] = { nullptr, nullptr, nullptr };
	context->PSSetShaderResources(0, 3, shaderResourceViews);

	ID3D11SamplerState *samplerStates[] = { nullptr, nullptr, nullptr };
	context->PSSetSamplers(0, 3, samplerStates);
}