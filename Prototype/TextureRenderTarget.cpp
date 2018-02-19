#include "pch.h"
#include "TextureRenderTarget.h"

using namespace OE;
using namespace DirectX;

TextureRenderTarget::TextureRenderTarget(uint32_t width, uint32_t height)
	: Texture()
	, m_width(width)
	, m_height(height)
	, m_renderTargetTexture(nullptr)
	, m_renderTargetView(nullptr)
{
}

void TextureRenderTarget::load(ID3D11Device *d3dDevice)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = m_width;
	textureDesc.Height = m_height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	ThrowIfFailed(d3dDevice->CreateTexture2D(&textureDesc, nullptr, m_renderTargetTexture.ReleaseAndGetAddressOf()),
		"Creating RenderTarget texture2D");

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	ThrowIfFailed(d3dDevice->CreateRenderTargetView(m_renderTargetTexture.Get(), &renderTargetViewDesc, m_renderTargetView.ReleaseAndGetAddressOf()),
		"Creating RenderTarget renderTargetView");

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	ThrowIfFailed(d3dDevice->CreateShaderResourceView(m_renderTargetTexture.Get(), &shaderResourceViewDesc, m_shaderResourceView.ReleaseAndGetAddressOf()), 
		"Creating RenderTarget shaderResourceView");
}

void TextureRenderTarget::unload()
{
	Texture::unload();

	m_renderTargetTexture.Reset();
	m_renderTargetView.Reset();
}
