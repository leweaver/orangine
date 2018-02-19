#include "pch.h"
#include "PBRMaterial.h"

using namespace OE;
using namespace DirectX;
using namespace std::literals;

PBRMaterial::PBRMaterial()
	: m_baseColor(XMVectorSet(0.f, 0.f, 0.f, 0.f))
	, m_sampleState(nullptr)
{
}

PBRMaterial::~PBRMaterial()
{
	if (m_sampleState)
		m_sampleState->Release();
	m_sampleState = nullptr;
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
	return settings;
}

bool PBRMaterial::createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer *&buffer)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PBRConstants);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	m_constants.viewProjection = SimpleMath::Matrix::Identity;
	m_constants.world = SimpleMath::Matrix::Identity;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &m_constants;
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
	m_constants.viewProjection = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projMatrix));
	m_constants.world = XMMatrixTranspose(worldMatrix);
	m_constants.baseColor = m_baseColor;

	context->UpdateSubresource(buffer, 0, nullptr, &m_constants, 0, 0);
}

void PBRMaterial::setContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();

	// TODO: Refactor into a re-usable method to load a texture into a field.
	// is this a duplicate of Material::ensureSamplerState ?
	if (m_baseColorTexture != nullptr)
	{
		if (!m_baseColorTexture->isValid())
		{
			try
			{
				m_baseColorTexture->load(device);
			}
			catch (std::exception &e)
			{
				LOG(WARNING) << "PBRMaterial: Failed to load baseColorTexture: "s + e.what();

				// TODO: Set to error texture?
				m_baseColorTexture = nullptr;
			}
		}
		if (m_baseColorTexture && m_baseColorTexture->isValid())
		{
			HRESULT hr = S_OK;
			if (!m_sampleState) {
				D3D11_SAMPLER_DESC samplerDesc;
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
				hr = device->CreateSamplerState(&samplerDesc, &m_sampleState);
			}

			if (SUCCEEDED(hr))
			{
				const auto shaderResourceView = m_baseColorTexture->getShaderResourceView();
				
				// Set shader texture resource in the pixel shader.
				context->PSSetShaderResources(0, 1, &shaderResourceView);
				context->PSSetSamplers(0, 1, &m_sampleState);
			}
			else
			{
				LOG(WARNING) << "PBRMaterial: Failed to create baseColorTexture sampler state with error code: "s + to_string(hr);

				// TODO: Set to error texture?
				m_baseColorTexture = nullptr;
			}
		}
	}
}

void PBRMaterial::unsetContextSamplers(const DX::DeviceResources &deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	ID3D11ShaderResourceView *shaderResourceViews[] = { nullptr };
	context->PSSetShaderResources(0, 1, shaderResourceViews);

	ID3D11SamplerState *samplerStates[] = { nullptr };
	context->PSSetSamplers(0, 1, samplerStates);
}