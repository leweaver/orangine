#include "pch.h"
#include "Material.h"

#include <d3dcompiler.h>
#include <locale>
#include <codecvt>
#include <sstream>

#include <comdef.h>
#include "Constants.h"
#include "RendererData.h"

using namespace OE;
using namespace DirectX;


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
	Release();
}

void Material::Release()
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

void Material::GetVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const {
	vertexAttributes.push_back(VertexAttribute::VA_POSITION);
	vertexAttributes.push_back(VertexAttribute::VA_COLOR);
}

const DXGI_FORMAT Material::format(VertexAttribute attribute)
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
	}

	throw std::logic_error("Material does not support format: " + VertexAttributeMeta::str(attribute));
}

UINT Material::inputSlot(VertexAttribute attribute)
{
	return attribute == VertexAttribute::VA_POSITION ? 0 : 1;
}

struct Constants
{
	DirectX::XMMATRIX m_viewProjection;
	DirectX::XMMATRIX m_world;
};

bool Material::Render(const RendererData &rendererData, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projMatrix, const DX::DeviceResources &deviceResources)
{
	if (m_errorState)
		return false;

	auto device = deviceResources.GetD3DDevice();
	auto context = deviceResources.GetD3DDeviceContext();
	const auto vsName = L"data/shaders/vertex_colors_VS.hlsl";
	const auto psName = L"data/shaders/vertex_colors_PS.hlsl";

	Constants constants;

	ID3DBlob* errorMsgs = nullptr;
	HRESULT hr;
	m_errorState = true;
	{

		if (!m_vertexShader) {
			ID3D10Blob *vertexShaderBytecode;
			hr = D3DCompileFromFile(vsName, nullptr, nullptr, "VSMain", "vs_4_0", 0, 0, &vertexShaderBytecode, &errorMsgs);
			if (!SUCCEEDED(hr)) {
				ThrowShaderError(hr, errorMsgs, vsName);
				Release();
				return false;
			}

			std::vector<VertexAttribute> attributes;
			GetVertexAttributes(attributes);
			std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;
			for (const auto &attr : attributes)
			{
				inputElementDesc.push_back(
					{ 
						VertexAttributeMeta::semanticName(attr), 
						VertexAttributeMeta::semanticIndex(attr), 
						format(attr), 
						inputSlot(attr),
						D3D11_APPEND_ALIGNED_ELEMENT, 
						D3D11_INPUT_PER_VERTEX_DATA, 
						0 
					}
				);
			}
			/*
			D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			const unsigned int numElements = inputElementDesc.size();
			*/
			device->CreateInputLayout(inputElementDesc.data(), static_cast<UINT>(inputElementDesc.size()), 
				vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), 
				&m_inputLayout);

			device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), nullptr, &m_vertexShader);

			vertexShaderBytecode->Release();

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(Constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			
			constants.m_viewProjection = Math::MAT4_IDENTITY;
			constants.m_world = worldMatrix;

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &constants;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

			device->CreateBuffer(&bufferDesc, &initData, &m_constantBuffer);
		}

		if (!m_pixelShader) {
			ID3D10Blob *m_pixelShaderBytecode;
			hr = D3DCompileFromFile(psName, nullptr, nullptr, "PSMain", "ps_4_0", 0, 0, &m_pixelShaderBytecode, &errorMsgs);
			if (!SUCCEEDED(hr)) {
				ThrowShaderError(hr, errorMsgs, psName);
				Release();
				return false;
			}

			device->CreatePixelShader(m_pixelShaderBytecode->GetBufferPointer(), m_pixelShaderBytecode->GetBufferSize(), nullptr, &m_pixelShader);
			m_pixelShaderBytecode->Release();
		}
	}
	m_errorState = false;

	// Update constant buffers

	// Convert to LH, for DirectX.
	constants.m_viewProjection = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projMatrix));
	constants.m_world = XMMatrixTranspose(worldMatrix);
	context->UpdateSubresource(m_constantBuffer, 0, nullptr, &constants, 0, 0);
	context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

	// We have a valid shader
	context->IASetInputLayout(m_inputLayout);
	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->PSSetShader(m_pixelShader, nullptr, 0);

	return true;
}

void Material::ThrowShaderError(HRESULT hr, ID3D10Blob* errorMessage, const wchar_t* shaderFilename)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	std::stringstream ss;

	const std::string shaderFilenameUtf8 = converter.to_bytes(std::wstring(shaderFilename));
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
		ss << converter.to_bytes(std::wstring(err.ErrorMessage()));
	}

	throw std::runtime_error(ss.str());
}
