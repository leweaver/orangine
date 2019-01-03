#include "pch.h"
#include "Deferred_light_material.h"
#include "Render_pass_config.h"
#include "Render_light_data.h"

#include <array>

using namespace oe;
using namespace std::literals;
using namespace DirectX;
using namespace SimpleMath;

const auto g_mrt_shader_resource_count = 9;
const auto g_mrt_sampler_state_count = 6;

const std::array<ID3D11ShaderResourceView*, g_mrt_shader_resource_count> g_nullShaderResourceViews = { 
	nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr 
};
const std::array<ID3D11SamplerState*, g_mrt_sampler_state_count> g_nullSamplerStates = { 
	nullptr, nullptr, nullptr, 
	nullptr, nullptr 
};

const std::string g_material_type = "Deferred_light_material";

const std::string& Deferred_light_material::materialType() const
{
	return g_material_type;
}

void Deferred_light_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	assert(blendMode == Render_pass_blend_mode::Additive);
}

Material::Shader_compile_settings Deferred_light_material::pixelShaderSettings() const
{
	auto settings = Base_type::pixelShaderSettings();

	if (_iblEnabled)
		settings.defines["MAP_IBL"] = "1";

	if (_shadowArrayEnabled)
		settings.defines["MAP_SHADOWMAP_ARRAY"] = "1";

	//settings.defines["DEBUG_DISPLAY_METALLIC_ROUGHNESS"] = "1";
	//settings.defines["DEBUG_LIGHTING_ONLY"] = "1";
	//settings.defines["DEBUG_NO_LIGHTING"] = "1";
	//settings.defines["DEBUG_DISPLAY_NORMALS"] = "1";
	return settings;
}

void Deferred_light_material::setupEmitted(bool enabled)
{
	_emittedEnabled = enabled;
}

void Deferred_light_material::updatePSConstantBufferValues(Deferred_light_material_constant_buffer& constants,
	const Render_light_data& renderlightData,
	const Matrix& worldMatrix,
	const Matrix& viewMatrix,
	const Matrix& projMatrix)
{
	const auto viewMatrixInv = viewMatrix.Invert();

	// Convert to LH, for DirectX. 
	constants.viewMatrixInv = XMMatrixTranspose(viewMatrixInv);
	constants.projMatrixInv = XMMatrixTranspose(XMMatrixInverse(nullptr, projMatrix));
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
	constants.eyePosition = Vector4(viewMatrixInv._41, viewMatrixInv._42, viewMatrixInv._43, 0.0);
	constants.emittedEnabled = _emittedEnabled;
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
	
	if (!_shadowMapSamplerState)
		validSamplers &= ensureSamplerState(deviceResources, *_shadowMapDepthTexture.get(), D3D11_TEXTURE_ADDRESS_CLAMP, &_shadowMapSamplerState);

	if (_shadowMapStencilTexture)
		_shadowMapStencilTexture->load(deviceResources.GetD3DDevice());

	if (validSamplers)
	{
		std::array<ID3D11ShaderResourceView*, g_mrt_shader_resource_count> shaderResourceViews = {
			_color0Texture->getShaderResourceView(),
			_color1Texture->getShaderResourceView(),
			_color2Texture->getShaderResourceView(),
			_depthTexture->getShaderResourceView(),
		};
		std::array<ID3D11SamplerState*, g_mrt_sampler_state_count> samplerStates = {
			_color0SamplerState.Get(),
			_color1SamplerState.Get(),
			_color2SamplerState.Get(),
			_depthSamplerState.Get(),
		};

		auto srvCount = 4;
		auto ssCount = 4;
		assert(shaderResourceViews[srvCount] == nullptr && samplerStates[ssCount] == nullptr);

		if (_iblEnabled) {
			if (!(renderLightData.environmentMapDiffuseSRV() && renderLightData.environmentMapSpecularSRV() && renderLightData.environmentMapSamplerState())) {
				throw std::runtime_error("Deferred_light_material requires a valid environment map be provided in the Render_light_data");
			}

			shaderResourceViews[srvCount++] = renderLightData.environmentMapBrdfSRV();
			shaderResourceViews[srvCount++] = renderLightData.environmentMapDiffuseSRV();
			shaderResourceViews[srvCount++] = renderLightData.environmentMapSpecularSRV();
			samplerStates[ssCount++] = renderLightData.environmentMapSamplerState();
		}

		if (_shadowArrayEnabled) {
			if (!(_shadowMapStencilTexture && _shadowMapDepthTexture)) {
				throw std::logic_error("Cannot bind a shadowmap stencil texture without a shadowmap depth texture.");
			}
			
			shaderResourceViews[srvCount++] = _shadowMapDepthTexture->getShaderResourceView();
			shaderResourceViews[srvCount++] = _shadowMapStencilTexture->getShaderResourceView();
			samplerStates[ssCount++] = _shadowMapSamplerState.Get();
		}

		// Set shader texture resource in the pixel shader.
		context->PSSetShaderResources(0, srvCount, shaderResourceViews.data());
		context->PSSetSamplers(0, ssCount, samplerStates.data());
	}
}

void Deferred_light_material::unsetContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	context->PSSetShaderResources(0, g_mrt_shader_resource_count, g_nullShaderResourceViews.data());
	context->PSSetSamplers(0, g_mrt_sampler_state_count, g_nullSamplerStates.data());
}