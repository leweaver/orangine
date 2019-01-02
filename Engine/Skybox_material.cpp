#include "pch.h"
#include "Skybox_material.h"

#include "Texture.h"

using namespace oe;

const std::string g_material_type = "Skybox_material";

// Uses blended alpha, to ensure standard rendering pipeline (not deferred)
Skybox_material::Skybox_material()
	: Material_base(Material_alpha_mode::Opaque, Material_face_cull_mode::Frontface)
{	
}

const std::string& Skybox_material::materialType() const
{
	return g_material_type;
}

void Skybox_material::releaseShaderResources()
{
	_samplerState.Reset();
}

void Skybox_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	auto device = deviceResources.GetD3DDevice();
	   
	// Release any previous samplerState and shaderResourceView objects
	releaseShaderResources();

	// Set sampler state
	D3D11_SAMPLER_DESC samplerDesc;

	if (_cubemapTexture && !_cubemapTexture->isValid()) {
		try
		{
			_cubemapTexture->load(device);
		}
		catch (std::exception& e)
		{
			LOG(WARNING) << std::string("Skybox_material: Failed to load texture") + e.what();
		}

		if (!_cubemapTexture->isValid()) {
			// TODO: Set to error texture?
			_cubemapTexture = nullptr;
		}
	}

	if (_cubemapTexture) {
		_shaderResourceView = _cubemapTexture->getShaderResourceView();
		assert(_shaderResourceView);

		// TODO: Use values from glTF?
		// Create a texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		// This call makes an AddRef call.
		ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &_samplerState));
	}
}

void Skybox_material::setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData)
{
	auto context = deviceResources.GetD3DDeviceContext();

	// Set shader texture resources in the pixel shader.
	context->PSSetShaderResources(0, 1, _shaderResourceView.GetAddressOf());
	context->PSSetSamplers(0, 1, _samplerState.GetAddressOf());
}

void Skybox_material::unsetContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	std::array<ID3D11ShaderResourceView*, 1> nullShaderResources = {nullptr};
	context->PSSetShaderResources(0, 1, nullShaderResources.data());

	std::array<ID3D11SamplerState*, 1> nullStates = { nullptr };
	context->PSSetSamplers(0, 1, nullStates.data());
}