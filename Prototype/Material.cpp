#include "pch.h"
#include "Material.h"

#include <d3dcompiler.h>
#include <locale>
#include <sstream>

#include <comdef.h>
#include "Constants.h"
#include "RendererData.h"

using namespace OE;
using namespace DirectX;
using namespace std::literals;


Material::Material()
	: m_vertexShader(nullptr)
	, m_pixelShader(nullptr)
	, m_inputLayout(nullptr)
	, m_constantBuffer(nullptr)
	, m_errorState(false)
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

	if (m_constantBuffer)
	{
		m_constantBuffer->Release();
		m_constantBuffer = nullptr;
	}
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
		std::set<std::string>(),
		std::set<std::string>()
	};
}

Material::ShaderCompileSettings Material::pixelShaderSettings() const
{
	return ShaderCompileSettings
	{
		L"data/shaders/vertex_colors_PS.hlsl"s,
		"PSMain"s,
		std::set<std::string>(),
		std::set<std::string>()
	};
}

bool Material::createVertexShader(ID3D11Device* device)
{
	ID3DBlob* errorMsgs;
	HRESULT hr;
	ID3D10Blob *vertexShaderBytecode;
	auto settings = vertexShaderSettings();
	hr = D3DCompileFromFile(settings.filename.c_str(), 
	                        nullptr, nullptr, 
	                        settings.entryPoint.c_str(), 
	                        "vs_4_0", 
	                        0, 0, 
	                        &vertexShaderBytecode, &errorMsgs);

	if (!SUCCEEDED(hr)) {
		throwShaderError(hr, errorMsgs, settings);
		release();
		return false;
	}

	std::vector<VertexAttribute> attributes;
	getVertexAttributes(attributes);
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;

	LOG(INFO) << "Adding vertex attr's";
	for (const auto &attr : attributes)
	{
		const auto semanticName = VertexAttributeMeta::semanticName(attr);
		LOG(INFO) << semanticName << " to slot " << inputSlot(attr);
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

	device->CreateInputLayout(inputElementDesc.data(), static_cast<UINT>(inputElementDesc.size()), 
	                          vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 
	                          &m_inputLayout);

	device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), nullptr, &m_vertexShader);

	vertexShaderBytecode->Release();
	return true;
}

bool Material::createPixelShader(ID3D11Device* device)
{
	ID3DBlob* errorMsgs;
	HRESULT hr;
	ID3D10Blob *pixelShaderBytecode;

	auto settings = pixelShaderSettings();
	hr = D3DCompileFromFile(settings.filename.c_str(),
	                        nullptr, nullptr,
	                        settings.entryPoint.c_str(),
	                        "ps_4_0",
	                        0, 0,
	                        &pixelShaderBytecode, &errorMsgs);
	if (!SUCCEEDED(hr)) {
		throwShaderError(hr, errorMsgs, settings);
		release();
		return false;
	}

	device->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), nullptr, &m_pixelShader);
	pixelShaderBytecode->Release();

	return true;
}

bool Material::render(const RendererData &rendererData, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projMatrix, const DX::DeviceResources &deviceResources)
{
	if (m_errorState)
		return false;

	const auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();
	
	m_errorState = true;
	{
		if (!m_vertexShader) {
			if (!createVertexShader(device)) 
				return false;

			createConstantBuffer(device, m_constantBuffer);
		}

		if (!m_pixelShader) {
			if (!createPixelShader(device)) 
				return false;
		}
	}
	m_errorState = false;

	// Update constant buffers
	updateConstantBuffer(worldMatrix, viewMatrix, projMatrix, context, m_constantBuffer);
	context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

	// We have a valid shader
	context->IASetInputLayout(m_inputLayout);
	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->PSSetShader(m_pixelShader, nullptr, 0);

	setContextSamplers(deviceResources);

	return true;
}

void Material::throwShaderError(HRESULT hr, ID3D10Blob* errorMessage, const ShaderCompileSettings &compileSettings)
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

	throw std::runtime_error(ss.str());
}
