#include "pch.h"
#include "PBRMaterial.h"
#include <minwinbase.h>

using namespace OE;
using namespace DirectX;
using namespace std::literals;

PBRMaterial::PBRMaterial()
	: m_baseColor(XMVectorSet(0.f, 0.f, 0.f, 0.f))
	, m_metallic(1.0)
	, m_roughness(0.0)
	, m_boundTextureCount(0)
{
	ZeroMemory(m_textures, sizeof(m_textures));
	ZeroMemory(m_shaderResourceViews, sizeof(m_shaderResourceViews));
	ZeroMemory(m_samplerStates, sizeof(m_samplerStates));
}

PBRMaterial::~PBRMaterial()
{
	releaseBindings();
}

void PBRMaterial::getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const {
	vertexAttributes.push_back(VertexAttribute::VA_POSITION);
	vertexAttributes.push_back(VertexAttribute::VA_NORMAL);
	vertexAttributes.push_back(VertexAttribute::VA_TANGENT);
	vertexAttributes.push_back(VertexAttribute::VA_TEXCOORD_0);
}

UINT PBRMaterial::inputSlot(VertexAttribute attribute)
{
	switch (attribute)
	{
	case VertexAttribute::VA_POSITION:
		return 0;
	case VertexAttribute::VA_NORMAL:
		return 1;
	case VertexAttribute::VA_TANGENT:
		return 2;
	case VertexAttribute::VA_TEXCOORD_0:
		return 3;
	default:
		throw std::runtime_error("Unsupported attribute");
	}
}

Material::ShaderCompileSettings PBRMaterial::vertexShaderSettings() const
{
	ShaderCompileSettings settings = Material::vertexShaderSettings();
	settings.filename = L"data/shaders/pbr_metallic_VS.hlsl"s;
	return settings;
}

Material::ShaderCompileSettings PBRMaterial::pixelShaderSettings() const
{
	ShaderCompileSettings settings = Material::pixelShaderSettings();
	settings.filename = L"data/shaders/pbr_metallic_PS.hlsl"s;
	
	if (m_textures[BaseColor])
		settings.defines["MAP_BASECOLOR"] = "1";
	if (m_textures[MetallicRoughness])
		settings.defines["MAP_METALLIC_ROUGHNESS"] = "1";
	if (m_textures[Normal])
		settings.defines["MAP_NORMAL"] = "1";

	return settings;
}

bool PBRMaterial::createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer *&buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PBRConstantsVS);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	m_constantsVS.worldViewProjection = SimpleMath::Matrix::Identity;
	m_constantsVS.world = SimpleMath::Matrix::Identity;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &m_constantsVS;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	device->CreateBuffer(&bufferDesc, &initData, &buffer);

	std::string name("PBRMaterial Constant Buffer");
	buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());

	return true;
}

void PBRMaterial::updateVSConstantBuffer(const SimpleMath::Matrix &worldMatrix, 
	const SimpleMath::Matrix &viewMatrix, 
	const SimpleMath::Matrix &projMatrix, 
	ID3D11DeviceContext *context, 
	ID3D11Buffer *buffer)
{
	// Convert to LH, for DirectX.
	m_constantsVS.worldViewProjection = XMMatrixMultiplyTranspose(worldMatrix, XMMatrixMultiply(viewMatrix, projMatrix));

	m_constantsVS.world = XMMatrixTranspose(worldMatrix);
	m_constantsVS.worldInvTranspose = XMMatrixInverse(nullptr, worldMatrix);

	context->UpdateSubresource(buffer, 0, nullptr, &m_constantsVS, 0, 0);
}

bool PBRMaterial::createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer *&buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PBRConstantsPS);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	m_constantsPS.world = SimpleMath::Matrix::Identity;
	m_constantsPS.baseColor = SimpleMath::Color(Colors::White);
	m_constantsPS.metallicRoughness = SimpleMath::Vector4::Zero;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &m_constantsPS;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	device->CreateBuffer(&bufferDesc, &initData, &buffer);

	std::string name("PBRMaterial Constant Buffer");
	buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());

	return true;
}

void PBRMaterial::updatePSConstantBuffer(const SimpleMath::Matrix &worldMatrix,
	const SimpleMath::Matrix &viewMatrix,
	const SimpleMath::Matrix &projMatrix,
	ID3D11DeviceContext *context,
	ID3D11Buffer *buffer)
{
	// Convert to LH, for DirectX.
	m_constantsPS.world = XMMatrixTranspose(worldMatrix);
	m_constantsPS.baseColor = m_baseColor;
	m_constantsPS.metallicRoughness = SimpleMath::Vector4(m_metallic, m_roughness, 0.0, 0.0);

	context->UpdateSubresource(buffer, 0, nullptr, &m_constantsPS, 0, 0);
}

void PBRMaterial::createShaderResources(const DX::DeviceResources &deviceResources)
{
	auto device = deviceResources.GetD3DDevice();

	assert(sizeof(m_shaderResourceViews) == (sizeof(ID3D11SamplerState*) * NumTextureTypes));

	// Release any previous samplerState and shaderResourceView objects
	releaseBindings();
	
	for (int t = 0; t < NumTextureTypes; ++t)
	{
		// Only initialize if a texture has been bound to this slot.
		const auto &texture = m_textures[t];
		if (texture == nullptr)
			continue;

		if (!texture->isValid())
		{
			try
			{
				texture->load(device);
			}
			catch (std::exception &e)
			{
				LOG(WARNING) << "PBRMaterial: Failed to load texture (type " + to_string(t) + "): "s + e.what();
			}

			if (!texture->isValid()) {
				// TODO: Set to error texture?
				m_textures[t] = nullptr;
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
		HRESULT hr = device->CreateSamplerState(&samplerDesc, &m_samplerStates[t]);
		if (SUCCEEDED(hr))
		{
			m_samplerStates[t]->AddRef();

			m_shaderResourceViews[t] = texture->getShaderResourceView();
			assert(m_shaderResourceViews[t]);

			m_shaderResourceViews[t]->AddRef();
			m_boundTextureCount++;
		}
		else
		{
			LOG(WARNING) << "PBRMaterial: Failed to create baseColorTexture sampler state with error code: "s + to_string(hr);

			// TODO: Set to error texture?
			m_textures[t] = nullptr;
		}
	}
}

void PBRMaterial::releaseBindings()
{
	for (unsigned int i = 0; i < NumTextureTypes; ++i)
	{
		if (m_shaderResourceViews[i]) {
			m_shaderResourceViews[i]->Release();
			m_shaderResourceViews[i] = nullptr;
		}

		if (m_samplerStates[i]) {
			m_samplerStates[i]->Release();
			m_samplerStates[i] = nullptr;
		}
	}

	m_boundTextureCount = 0;
}

void PBRMaterial::setContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();
	
	// Set shader texture resources in the pixel shader.
	context->PSSetShaderResources(0, m_boundTextureCount, m_shaderResourceViews);
	context->PSSetSamplers(0, m_boundTextureCount, m_samplerStates);
}

void PBRMaterial::unsetContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	ID3D11ShaderResourceView *shaderResourceViews[] = { nullptr };
	ID3D11SamplerState *samplerStates[] = { nullptr };

	// TODO: Is it faster to statically allocate an array of size NumTextures?
	for (unsigned int i = 0; i < m_boundTextureCount; ++i) {
		context->PSSetShaderResources(i, 1, shaderResourceViews);
		context->PSSetSamplers(i, 1, samplerStates);
	}
}