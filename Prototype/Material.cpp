#include "pch.h"
#include "Material.h"

#include <d3dcompiler.h>
#include <locale>
#include <sstream>

#include <comdef.h>
#include "RendererData.h"
#include "Texture.h"

using namespace OE;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;


Material::Material()
	: m_vertexShader(nullptr)
	, m_pixelShader(nullptr)
	, m_inputLayout(nullptr)
	, m_vsConstantBuffer(nullptr)
	, m_errorState(false)
	, m_requiresRecompile(true)
	, m_alphaMode(MaterialAlphaMode::OPAQUE)
{
}

Material::~Material()
{
	release();
}

void Material::release()
{
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}

	if (m_inputLayout)
	{
		m_inputLayout->Release();
		m_inputLayout = nullptr;
	}

	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	m_blendState.Reset();
	m_vsConstantBuffer.Reset();
	m_psConstantBuffer.Reset();
}

void Material::getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const {
	vertexAttributes.push_back(VertexAttribute::VA_POSITION);
	vertexAttributes.push_back(VertexAttribute::VA_COLOR);
}

DXGI_FORMAT Material::format(VertexAttribute attribute)
{
	switch (attribute)
	{
	case VertexAttribute::VA_TEXCOORD_0:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;

	case VertexAttribute::VA_POSITION:
	case VertexAttribute::VA_COLOR:
	case VertexAttribute::VA_NORMAL:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;

	case VertexAttribute::VA_TANGENT:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;

	default:
		throw std::logic_error("Material does not support format: " + VertexAttributeMeta::str(attribute));
	}
}

UINT Material::inputSlot(VertexAttribute attribute)
{
	return attribute == VertexAttribute::VA_POSITION ? 0 : 1;
}

Material::ShaderCompileSettings Material::vertexShaderSettings() const
{
	return ShaderCompileSettings
	{
		L"data/shaders/vertex_colors_VS.hlsl"s,
		"VSMain"s,
		std::map<std::string, std::string>(),
		std::set<std::string>()
	};
}

Material::ShaderCompileSettings Material::pixelShaderSettings() const
{
	return ShaderCompileSettings
	{
		L"data/shaders/vertex_colors_PS.hlsl"s,
		"PSMain"s,
		std::map<std::string, std::string>(),
		std::set<std::string>()
	};
}

bool Material::createVertexShader(ID3D11Device* device)
{
	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
	ID3DBlob* vertexShaderBytecode;
	auto settings = vertexShaderSettings();

	std::vector<VertexAttribute> attributes;
	getVertexAttributes(attributes);
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;

	LOG(INFO) << "Adding vertex attr's";
	for (const auto &attr : attributes)
	{
		const auto semanticName = VertexAttributeMeta::semanticName(attr);
		LOG(INFO) << "  " << semanticName << " to slot " << inputSlot(attr);
		inputElementDesc.push_back(
			{
				semanticName,
				VertexAttributeMeta::semanticIndex(attr),
				format(attr),
				inputSlot(attr),
				D3D11_APPEND_ALIGNED_ELEMENT,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			}
		);
	}
	
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif

	hr = D3DCompileFromFile(settings.filename.c_str(), 
	                        nullptr, 
							D3D_COMPILE_STANDARD_FILE_INCLUDE,
	                        settings.entryPoint.c_str(), 
	                        "vs_5_0", 
							flags, 0,
	                        &vertexShaderBytecode, 
							errorMsgs.ReleaseAndGetAddressOf());

	if (!SUCCEEDED(hr)) {
		LOG(WARNING) << createShaderError(hr, errorMsgs.Get(), settings);
		release();
		return false;
	}
	hr = device->CreateInputLayout(inputElementDesc.data(), static_cast<UINT>(inputElementDesc.size()), 
	                          vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 
	                          &m_inputLayout);
	if (!SUCCEEDED(hr))
	{
		LOG(WARNING) << "Failed to create vertex input layout: " << std::to_string(hr);
		release();
		return false;
	}

	hr = device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), nullptr, &m_vertexShader);
	if (!SUCCEEDED(hr))
	{
		LOG(WARNING) << "Failed to create vertex shader: " << std::to_string(hr);
		release();
		return false;
	}

	return true;
}

bool Material::createPixelShader(ID3D11Device* device)
{
	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorMsgs;
	ID3DBlob* pixelShaderBytecode;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif


	auto settings = pixelShaderSettings();

	std::vector<D3D_SHADER_MACRO> defines;
	for (const auto &define : settings.defines)
	{
		defines.push_back({
			define.first.c_str(),
			define.second.c_str()
			});
	}
	defines.push_back({ nullptr, nullptr });

	hr = D3DCompileFromFile(settings.filename.c_str(),
		defines.data(),
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		settings.entryPoint.c_str(),
		"ps_5_0",
		flags, 0,
		&pixelShaderBytecode, 
		errorMsgs.ReleaseAndGetAddressOf());
	if (!SUCCEEDED(hr)) {
		LOG(WARNING) << createShaderError(hr, errorMsgs.Get(), settings);
		release();
		return false;
	}

	hr = device->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), nullptr, &m_pixelShader);
	if (!SUCCEEDED(hr))
	{
		LOG(WARNING) << "Failed to create vertex shader: " << std::to_string(hr);
		release();
		return false;
	}

	return true;
}
void Material::createBlendState(ID3D11Device *device, ID3D11BlendState *&blendState)
{
	D3D11_BLEND_DESC blendStateDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	device->CreateBlendState(&blendStateDesc, &blendState);
}

bool Material::render(const RendererData &rendererData, const Matrix &worldMatrix, const Matrix &viewMatrix, const Matrix &projMatrix, const DX::DeviceResources &deviceResources)
{
	const auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();

	if (m_requiresRecompile) {
		LOG(INFO) << "Recompiling shaders for material";

		m_requiresRecompile = false;
		release();

		m_errorState = true;
		{
			createShaderResources(deviceResources);
			
			if (!createVertexShader(device))
				return false;

			createVSConstantBuffer(device, *m_vsConstantBuffer.ReleaseAndGetAddressOf());

			if (!createPixelShader(device))
				return false;

			createPSConstantBuffer(device, *m_psConstantBuffer.ReleaseAndGetAddressOf());
			createBlendState(device, *m_blendState.ReleaseAndGetAddressOf());
		}
		m_errorState = false;
	}

	if (m_errorState)
		return false;

	// We have a valid shader
	context->IASetInputLayout(m_inputLayout);
	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->PSSetShader(m_pixelShader, nullptr, 0);

	// Update constant buffers
	if (m_vsConstantBuffer != nullptr) {
		updateVSConstantBuffer(worldMatrix, viewMatrix, projMatrix, context, m_vsConstantBuffer.Get());
		context->VSSetConstantBuffers(0, 1, m_vsConstantBuffer.GetAddressOf());
	}
	if (m_psConstantBuffer != nullptr) {
		updatePSConstantBuffer(worldMatrix, viewMatrix, projMatrix, context, m_psConstantBuffer.Get());
		context->PSSetConstantBuffers(0, 1, m_psConstantBuffer.GetAddressOf());
	}

	// Set texture samples
	setContextSamplers(deviceResources);

	// set blend state
	const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const UINT sampleMask = 0xffffffff;
	context->OMSetBlendState(m_blendState.Get(), blendFactor, sampleMask);

	return true;
}

void Material::unbind(const DX::DeviceResources& deviceResources)
{
	unsetContextSamplers(deviceResources);
}

std::string Material::createShaderError(HRESULT hr, ID3D10Blob* errorMessage, const ShaderCompileSettings &compileSettings)
{
	std::stringstream ss;

	const std::string shaderFilenameUtf8 = utf8_encode(compileSettings.filename);
	ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

	// Get a pointer to the error message text buffer.
	if (errorMessage != nullptr) {
		char *compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());
		ss << compileErrors << std::endl;
		errorMessage->Release();
	}
	else
	{
		_com_error err(hr);
		ss << utf8_encode(std::wstring(err.ErrorMessage()));
	}

	return ss.str();
}

bool Material::ensureSamplerState(const DX::DeviceResources &deviceResources, Texture &texture, ID3D11SamplerState **d3D11SamplerState)
{
	const auto device = deviceResources.GetD3DDevice();

	// TODO: Replace this method with that from PBRMaterial
	if (!texture.isValid())
	{
		try
		{
			texture.load(device);
		}
		catch (std::exception &e)
		{
			LOG(WARNING) << "Material: Failed to load texture: "s + e.what();
			return false;
		}
	}

	if (texture.isValid())
	{
		if (!*d3D11SamplerState) {
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
			ThrowIfFailed(device->CreateSamplerState(&samplerDesc, d3D11SamplerState));
		}
	}

	return true;
}
