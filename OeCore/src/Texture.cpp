#include "pch.h"

#include "OeCore/Texture.h"
#include "OeCore/EngineUtils.h"

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

using namespace oe;

void Texture::unload()
{
	_shaderResourceView.Reset();
}

void Buffer_texture::load(ID3D11Device* device)
{
	throw std::runtime_error("Not implemented");
}

void File_texture::load(ID3D11Device* device)
{

	unload();

	HRESULT hr;
	if (str_ends(_filename, L".dds"))
		hr = DirectX::CreateDDSTextureFromFile(device, _filename.c_str(), _textureResource.ReleaseAndGetAddressOf(), &_shaderResourceView);
	else
		hr = DirectX::CreateWICTextureFromFile(device, _filename.c_str(), _textureResource.ReleaseAndGetAddressOf(), &_shaderResourceView);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	_shaderResourceView->GetDesc(&desc);

	ThrowIfFailed(hr, utf8_encode(_filename));
}

void File_texture::unload()
{
	Texture::unload();
	_textureResource.Reset();
}

Depth_texture::Depth_texture(const DX::DeviceResources& deviceResources)
	: _deviceResources(deviceResources)
{
}

void Depth_texture::load(ID3D11Device* device)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = -1;

	ThrowIfFailed(device->CreateShaderResourceView(_deviceResources.GetDepthStencil(), &sr_desc, _shaderResourceView.ReleaseAndGetAddressOf()),
		"DepthTexture shaderResourceView");
}

void oe::to_json(nlohmann::json& j, const std::shared_ptr<Texture> texture) {
    // TODO: Return asset ID's here?
    j = nlohmann::json{ {"loaded", texture ? "yes" : "no" } };
}

void oe::from_json(const nlohmann::json& /*j*/, std::shared_ptr<Texture>& /*texture*/) {
    throw std::runtime_error("Unsupported - texture deserialization");
}
