#include "pch.h"
#include "PBR_material.h"
#include "Render_pass_config.h"

using namespace oe;
using namespace DirectX;
using namespace std::literals;

const std::array<ID3D11ShaderResourceView*, 5> g_nullShaderResourceViews = { nullptr, nullptr, nullptr, nullptr, nullptr };
const std::array<ID3D11SamplerState*, 5> g_nullSamplerStates = { nullptr, nullptr, nullptr, nullptr, nullptr };

const std::string g_material_type = "PBR_material";

PBR_material::PBR_material()
	: _enableDeferred(false)
	, _baseColor(SimpleMath::Vector4::One)
	, _metallic(1.0)
	, _roughness(1.0)
	, _emissive(0, 0, 0)
	, _alphaCutoff(0.5)
	, _boundTextureCount(0)
{
	std::fill(_textures.begin(), _textures.end(), nullptr);
	std::fill(_shaderResourceViews.begin(), _shaderResourceViews.end(), nullptr);
	std::fill(_samplerStates.begin(), _samplerStates.end(), nullptr);
}

PBR_material::~PBR_material()
{
	releaseShaderResources();
}

const std::string& PBR_material::materialType() const
{
	return g_material_type;
}

Material::Shader_compile_settings PBR_material::pixelShaderSettings() const
{
	auto settings = Base_type::pixelShaderSettings();

	if (_textures[BaseColor])
		settings.defines["MAP_BASECOLOR"] = "1";
	if (_textures[MetallicRoughness])
		settings.defines["MAP_METALLIC_ROUGHNESS"] = "1";
	if (_textures[Normal])
		settings.defines["MAP_NORMAL"] = "1";
	if (_textures[Emissive])
		settings.defines["MAP_EMISSIVE"] = "1";
	if (_textures[Occlusion])
		settings.defines["MAP_OCCLUSION"] = "1";

	if (_enableDeferred)
		settings.defines["PS_PIPELINE_DEFERRED"] = "1";
	else
		settings.defines["PS_PIPELINE_STANDARD"] = "1";

	if (getAlphaMode() == Material_alpha_mode::Mask)
		settings.defines["ALPHA_MASK_VALUE"] = std::to_string(alphaCutoff());

	return settings;
}

void PBR_material::updateVSConstantBufferValues(PBR_material_vs_constant_buffer& constants,
	const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& viewMatrix,
	const SimpleMath::Matrix& projMatrix)
{
	// Note that HLSL matrices are Column Major (as opposed to Row Major in DirectXMath) - so we need to transpose everything.
	constants.worldView = XMMatrixMultiplyTranspose(worldMatrix, viewMatrix);

	constants.world = XMMatrixTranspose(worldMatrix);
	constants.worldInvTranspose = XMMatrixInverse(nullptr, worldMatrix);
}

void PBR_material::updatePSConstantBufferValues(PBR_material_ps_constant_buffer& constants,
	const Render_light_data& renderlightData,
	const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& viewMatrix,
	const SimpleMath::Matrix& projMatrix)
{
	// Convert to LH, for DirectX.
	constants.world = XMMatrixTranspose(worldMatrix);
	constants.baseColor = _baseColor;
	constants.metallicRoughness = SimpleMath::Vector4(_metallic, _roughness, 0.0, 0.0);
	constants.emissive = SimpleMath::Vector4(_emissive.x, _emissive.y, _emissive.z, 0.0);
	constants.eyePosition = SimpleMath::Vector4(worldMatrix._41, worldMatrix._42, worldMatrix._43, 0.0);
}

void PBR_material::createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode)
{
	// TODO: This is a hacky way of finding this information out.
	_enableDeferred = blendMode == Render_pass_blend_mode::Opaque;

	static_assert(sizeof(_shaderResourceViews) == (sizeof(ID3D11SamplerState*) * NumTextureTypes));

	auto device = deviceResources.GetD3DDevice();

	// Release any previous samplerState and shaderResourceView objects
	releaseShaderResources();

	for (auto t = 0; t < NumTextureTypes; ++t)
	{
		// Only initialize if a texture has been bound to this slot.
		const auto& texture = _textures[t];
		if (texture == nullptr)
			continue;

		if (!texture->isValid())
		{
			try
			{
				texture->load(device);
			}
			catch (std::exception& e)
			{
				LOG(WARNING) << "PBR_Material: Failed to load texture (type " + std::to_string(t) + "): "s + e.what();
			}

			if (!texture->isValid()) {
				// TODO: Set to error texture?
				_textures[t] = nullptr;
				continue;
			}
		}

		// Set sampler state
		D3D11_SAMPLER_DESC samplerDesc;

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
		const auto hr = device->CreateSamplerState(&samplerDesc, &_samplerStates[t]);
		if (SUCCEEDED(hr))
		{
			_shaderResourceViews[t] = texture->getShaderResourceView();
			assert(_shaderResourceViews[t]);

			_shaderResourceViews[t]->AddRef();
			_boundTextureCount++;
		}
		else
		{
			LOG(WARNING) << "PBR_Material: Failed to create samplerState with error code: "s + hr_to_string(hr);

			// TODO: Set to error texture?
			_textures[t] = nullptr;
		}
	}

	LOG(G3LOG_DEBUG) << "Created PBR_Material shader resources. Texture Count: " << std::to_string(_boundTextureCount);
}

void PBR_material::releaseShaderResources()
{
	for (unsigned int i = 0; i < NumTextureTypes; ++i)
	{
		if (_shaderResourceViews[i]) {
			_shaderResourceViews[i]->Release();
			_shaderResourceViews[i] = nullptr;
		}

		if (_samplerStates[i]) {
			_samplerStates[i]->Release();
			_samplerStates[i] = nullptr;
		}
	}

	_boundTextureCount = 0;
}

void PBR_material::setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData)
{
	auto context = deviceResources.GetD3DDeviceContext();

	// Set shader texture resources in the pixel shader.
	context->PSSetShaderResources(0, _boundTextureCount, _shaderResourceViews.data());
	context->PSSetSamplers(0, _boundTextureCount, _samplerStates.data());
}

void PBR_material::unsetContextSamplers(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	static_assert(g_nullShaderResourceViews.size() == NumTextureTypes);
	static_assert(g_nullSamplerStates.size() == NumTextureTypes);

	context->PSSetShaderResources(0, NumTextureTypes, g_nullShaderResourceViews.data());
	context->PSSetSamplers(0, NumTextureTypes, g_nullSamplerStates.data());
}
