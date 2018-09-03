#include "pch.h"

#include "Shadow_map_texture.h"

using namespace oe;
using namespace Microsoft::WRL;

Shadow_map_texture::Shadow_map_texture(uint32_t width, uint32_t height) 
	: _width(width)
	, _height(height)
	, _depthStencilView({})
{}

void Shadow_map_texture::load(ID3D11Device* device)
{
	// Initialize the render target texture description.
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = _width;
	textureDesc.Height = _height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	ComPtr<ID3D11Texture2D> shadowMapTexture;
	ThrowIfFailed(device->CreateTexture2D(&textureDesc, nullptr, &shadowMapTexture),
		"Creating Shadow_map_texture texture2D");

	// depth stencil resource view desc.
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	depthStencilViewDesc.Flags = 0;
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateDepthStencilView(shadowMapTexture.Get(), &depthStencilViewDesc, &_depthStencilView),
		"Creating Shadow_map_texture depthStencilView");
	
	// shader resource view desc.
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

	// Create the shader resource view.
	ThrowIfFailed(device->CreateShaderResourceView(shadowMapTexture.Get(), &shaderResourceViewDesc, &_shaderResourceView),
		"Creating Shadow_map_texture shaderResourceView");
}

void Shadow_map_texture::unload()
{
	_depthStencilView.Reset();
	Texture::unload();
}
