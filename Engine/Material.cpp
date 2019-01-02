#include "pch.h"
#include "Material.h"

#include <d3dcompiler.h>
#include <locale>
#include <sstream>

#include <comdef.h>
#include "Renderer_data.h"
#include "Render_light_data.h"
#include "Texture.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

const std::wstring Material::shader_path = std::wstring(L"data/shaders/");

Material::Material(Material_alpha_mode alphaMode, Material_face_cull_mode faceCullMode)
	: _vertexShader(nullptr)
	, _pixelShader(nullptr)
	, _inputLayout(nullptr)
	, _errorState(false)
	, _requiresRecompile(true)
	, _alphaMode(alphaMode)
	, _faceCullMode(faceCullMode)
{
}

Material::~Material()
{
	release();
}

void Material::release()
{
	if (_vertexShader)
	{
		_vertexShader->Release();
		_vertexShader = nullptr;
	}

	if (_inputLayout)
	{
		_inputLayout->Release();
		_inputLayout = nullptr;
	}

	if (_pixelShader)
	{
		_pixelShader->Release();
		_pixelShader = nullptr;
	}

	_vsConstantBuffer.Reset();
	_psConstantBuffer.Reset();
}

void Material::vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const {
	vertexAttributes.push_back(Vertex_attribute::Position);
	vertexAttributes.push_back(Vertex_attribute::Color);
}

DXGI_FORMAT Material::format(Vertex_attribute attribute)
{
	switch (attribute)
	{
	case Vertex_attribute::Texcoord_0:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;

	case Vertex_attribute::Position:
	case Vertex_attribute::Color:
	case Vertex_attribute::Normal:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;

	case Vertex_attribute::Tangent:
		return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;

	default:
		throw std::logic_error("Material does not support format: "s.append(Vertex_attribute_meta::str(attribute)));
	}
}

UINT Material::inputSlot(Vertex_attribute attribute)
{
	return attribute == Vertex_attribute::Position ? 0 : 1;
}

Material::Shader_compile_settings Material::vertexShaderSettings() const
{
	return Shader_compile_settings
	{
		L"data/shaders/vertex_colors_VS.hlsl"s,
		"VSMain"s,
		std::map<std::string, std::string>(),
		std::set<std::string>()
	};
}

Material::Shader_compile_settings Material::pixelShaderSettings() const
{
	return Shader_compile_settings
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

	std::vector<Vertex_attribute> attributes;
	vertexAttributes(attributes);
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;

	LOG(G3LOG_DEBUG) << "Adding vertex attr's";
	for (const auto& attr : attributes)
	{
		const auto semanticName = Vertex_attribute_meta::semanticName(attr);
		LOG(G3LOG_DEBUG) << "  " << semanticName << " to slot " << inputSlot(attr);
		inputElementDesc.push_back(
			{
				semanticName.data(),
				Vertex_attribute_meta::semanticIndex(attr),
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
	                          &_inputLayout);
	if (!SUCCEEDED(hr))
	{
		LOG(WARNING) << "Failed to create vertex input layout: " << std::to_string(hr);
		release();
		return false;
	}

	hr = device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), nullptr, &_vertexShader);
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
	for (const auto& define : settings.defines)
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

	hr = device->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), nullptr, &_pixelShader);
	if (!SUCCEEDED(hr))
	{
		LOG(WARNING) << "Failed to create vertex shader: " << std::to_string(hr);
		release();
		return false;
	}

	auto debugName = "Material:" + utf8_encode(settings.filename);
	_pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(debugName.size()), debugName.c_str());

	return true;
}

void Material::bind(Render_pass_blend_mode blendMode,
	const Render_light_data& renderLightData,
	const DX::DeviceResources& deviceResources,
	bool enablePixelShader)
{
	const auto device = deviceResources.GetD3DDevice();

	if (_requiresRecompile) {
		LOG(INFO) << "Recompiling shaders for material";

		_requiresRecompile = false;
		release();

		_errorState = true;
		{
			createShaderResources(deviceResources, renderLightData, blendMode);

			if (!createVertexShader(device))
				throw std::runtime_error("Failed to create vertex shader");

			if (!createVSConstantBuffer(device, *_vsConstantBuffer.ReleaseAndGetAddressOf()))
				throw std::runtime_error("Failed to create vertex shader constant buffer");

			if (!createPixelShader(device))
				throw std::runtime_error("Failed to create pixel shader");

			if (!createPSConstantBuffer(device, *_psConstantBuffer.ReleaseAndGetAddressOf()))
				throw std::runtime_error("Failed to create pixel shader constant buffer");
		}
		_errorState = false;
	}

	// We have a valid shader
	auto context = deviceResources.GetD3DDeviceContext();
	context->IASetInputLayout(_inputLayout);
	context->VSSetShader(_vertexShader, nullptr, 0);

	if (enablePixelShader) {
		context->PSSetShader(_pixelShader, nullptr, 0);

		// Set texture samples
		setContextSamplers(deviceResources, renderLightData);
	}
}

bool Material::render(const Renderer_data& rendererData,
	const Render_light_data& renderLightData,
	const Matrix& worldMatrix, 
	const Matrix& viewMatrix, 
	const Matrix& projMatrix, 
	const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();

	// Update constant buffers
	if (_vsConstantBuffer != nullptr) {
		updateVSConstantBuffer(worldMatrix, viewMatrix, projMatrix, context, _vsConstantBuffer.Get());
		context->VSSetConstantBuffers(0, 1, _vsConstantBuffer.GetAddressOf());
	}
	if (_psConstantBuffer != nullptr) {
		updatePSConstantBuffer(renderLightData, worldMatrix, viewMatrix, projMatrix, context, _psConstantBuffer.Get());
	}
	ID3D11Buffer* psConstantBuffers[] = { _psConstantBuffer.Get(), renderLightData.buffer() };
	context->PSSetConstantBuffers(0, 2, psConstantBuffers);
	
	// Render the triangles
	if (rendererData.indexBufferAccessor != nullptr)
	{
		context->DrawIndexed(rendererData.indexCount, 0, 0);
	}
	else
	{
		context->Draw(rendererData.vertexCount, rendererData.vertexCount);
	}

	return true;
}

void Material::unbind(const DX::DeviceResources& deviceResources)
{
	auto context = deviceResources.GetD3DDeviceContext();
	unsetContextSamplers(deviceResources);

	context->IASetInputLayout(nullptr);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

std::string Material::createShaderError(HRESULT hr, ID3D10Blob* errorMessage, const Shader_compile_settings& compileSettings)
{
	std::stringstream ss;

	const std::string shaderFilenameUtf8 = utf8_encode(compileSettings.filename);
	ss << "Error compiling shader \"" << shaderFilenameUtf8 << "\"" << std::endl;

	// Get a pointer to the error message text buffer.
	if (errorMessage != nullptr) {
		const auto compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());
		ss << compileErrors << std::endl;
	}
	else
	{
		_com_error err(hr);
		ss << utf8_encode(std::wstring(err.ErrorMessage()));
	}

	return ss.str();
}

bool Material::ensureSamplerState(const DX::DeviceResources& deviceResources, 
	Texture& texture, 
	D3D11_TEXTURE_ADDRESS_MODE textureAddressMode, 
	ID3D11SamplerState** d3D11SamplerState)
{
	const auto device = deviceResources.GetD3DDevice();

	// TODO: Replace this method with that from PBRMaterial
	if (!texture.isValid())
	{
		try
		{
			texture.load(device);
		}
		catch (std::exception& e)
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
			samplerDesc.AddressU = textureAddressMode;
			samplerDesc.AddressV = textureAddressMode;
			samplerDesc.AddressW = textureAddressMode;
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
