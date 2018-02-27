#include "pch.h"
#include "TextureImpl.h"
#include "DeviceResources.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

using namespace OE;

void Texture::unload()
{
	m_shaderResourceView.Reset();
}

void BufferTexture::load(ID3D11Device *device)
{
}

void FileTexture::load(ID3D11Device *device)
{
	HRESULT hr = S_OK;

	unload();

	if (str_ends(m_filename, L".dds"))
		hr = DirectX::CreateDDSTextureFromFile(device, m_filename.c_str(), m_textureResource.ReleaseAndGetAddressOf(), &m_shaderResourceView);
	else
		hr = DirectX::CreateWICTextureFromFile(device, m_filename.c_str(), m_textureResource.ReleaseAndGetAddressOf(), &m_shaderResourceView);
	
	ThrowIfFailed(hr, utf8_encode(m_filename));
}

void FileTexture::unload()
{
	Texture::unload();
	m_textureResource.Reset();
}

DepthTexture::DepthTexture(const DX::DeviceResources &deviceResources)
	: Texture()
	, m_deviceResources(deviceResources)
{
}

void DepthTexture::load(ID3D11Device *device)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = -1;

	ThrowIfFailed(device->CreateShaderResourceView(m_deviceResources.GetDepthStencil(), &sr_desc, m_shaderResourceView.ReleaseAndGetAddressOf()),
		"DepthTexture shaderResourceView");
}