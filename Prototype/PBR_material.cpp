#include "pch.h"
#include "PBR_material.h"
#include "Render_pass_info.h"

using namespace oe;
using namespace DirectX;
using namespace std::literals;

const std::array<ID3D11ShaderResourceView*, 5> g_nullShaderResourceViews = { nullptr, nullptr, nullptr, nullptr, nullptr };
const std::array<ID3D11SamplerState*, 5> g_nullSamplerStates = { nullptr, nullptr, nullptr, nullptr, nullptr };

PBR_material::PBR_material()
	: _enableDeferred(false)
	, _baseColor(SimpleMath::Vector4::One)
	, _metallic(1.0)
	, _roughness(1.0)
	, _emissive(0, 0, 0)
	, _alphaCutoff(0.5)
	, _boundTextureCount(0)
	, _samplerStates{}
	, _shaderResourceViews{}
{
	std::fill(_textures.begin(), _textures.end(), nullptr);
	std::fill(_shaderResourceViews.begin(), _shaderResourceViews.end(), nullptr);
	std::fill(_samplerStates.begin(), _samplerStates.end(), nullptr);
}

PBR_material::~PBR_material()
{
	releaseShaderResources();
}

void PBR_material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const {
	vertexAttributes.push_back(Vertex_attribute::Position);
	vertexAttributes.push_back(Vertex_attribute::Normal);
	vertexAttributes.push_back(Vertex_attribute::Tangent);
	vertexAttributes.push_back(Vertex_attribute::Texcoord_0);
}

UINT PBR_material::inputSlot(Vertex_attribute attribute)
{
	switch (attribute)
	{
	case Vertex_attribute::Position:
		return 0;
	case Vertex_attribute::Normal:
		return 1;
	case Vertex_attribute::Tangent:
		return 2;
	case Vertex_attribute::Texcoord_0:
		return 3;
	default:
		throw std::runtime_error("Unsupported attribute");
	}
}

Material::Shader_compile_settings PBR_material::vertexShaderSettings() const
{
	auto settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/pbr_metallic_VS.hlsl"s;
	return settings;
}

Material::Shader_compile_settings PBR_material::pixelShaderSettings() const
{
	auto settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/pbr_metallic_PS.hlsl"s;
	
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

bool PBR_material::createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PBR_constants_vs);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	_constantsVs.worldViewProjection = SimpleMath::Matrix::Identity;
	_constantsVs.world = SimpleMath::Matrix::Identity;
	_constantsVs.worldInvTranspose = SimpleMath::Matrix::Identity;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &_constantsVs;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

	std::string name("PBR_Material Constant Buffer");
	DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

	return true;
}

void PBR_material::updateVSConstantBuffer(const SimpleMath::Matrix& worldMatrix, 
	const SimpleMath::Matrix& viewMatrix, 
	const SimpleMath::Matrix& projMatrix, 
	ID3D11DeviceContext* context, 
	ID3D11Buffer* buffer)
{
	// Convert to LH, for DirectX.
	_constantsVs.worldViewProjection = XMMatrixMultiplyTranspose(worldMatrix, XMMatrixMultiply(viewMatrix, projMatrix));

	_constantsVs.world = XMMatrixTranspose(worldMatrix);
	_constantsVs.worldInvTranspose = XMMatrixInverse(nullptr, worldMatrix);

	context->UpdateSubresource(buffer, 0, nullptr, &_constantsVs, 0, 0);
}

bool PBR_material::createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PBR_constants_ps);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	_constantsPs.world = SimpleMath::Matrix::Identity;
	_constantsPs.baseColor = SimpleMath::Color(Colors::White);
	_constantsPs.metallicRoughness = SimpleMath::Vector4::One;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &_constantsPs;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

	std::string name("PBR_Material Constant Buffer");
	DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

	return true;
}

void PBR_material::updatePSConstantBuffer(const SimpleMath::Matrix& worldMatrix,
	const SimpleMath::Matrix& viewMatrix,
	const SimpleMath::Matrix& projMatrix,
	ID3D11DeviceContext* context,
	ID3D11Buffer* buffer)
{
	// Convert to LH, for DirectX.
	_constantsPs.world = XMMatrixTranspose(worldMatrix);
	_constantsPs.baseColor = _baseColor;
	_constantsPs.metallicRoughness = SimpleMath::Vector4(_metallic, _roughness, 0.0, 0.0);
	_constantsPs.emissive = SimpleMath::Vector4(_emissive.x, _emissive.y, _emissive.z, 0.0);
	_constantsPs.eyePosition = SimpleMath::Vector4(worldMatrix._41, worldMatrix._42, worldMatrix._43, 0.0);

	context->UpdateSubresource(buffer, 0, nullptr, &_constantsPs, 0, 0);
}

void PBR_material::createShaderResources(const DX::DeviceResources& deviceResources, Render_pass_output_format outputFormat)
{
	_enableDeferred = outputFormat == Render_pass_output_format::Shaded_DeferredLight;
	assert(_enableDeferred || outputFormat == Render_pass_output_format::Shaded_StandardLight);

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

void PBR_material::setContextSamplers(const DX::DeviceResources& deviceResources)
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