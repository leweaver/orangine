#include "pch.h"
#include "TextureImpl.h"
#include "DeviceResources.h"

using namespace OE;

Texture::~Texture()
{
	unload();
}

void Texture::unload()
{
	m_shaderResourceView.Reset();
}

HRESULT TextureBase::loadShaderResourceView(ID3D11Device *device, const DirectX::TexMetadata &metaData, const DirectX::ScratchImage &scratchImage)
{
	HRESULT hr = S_OK;
	const DirectX::ScratchImage *finalImage;

	DirectX::ScratchImage processedImage;
	if (m_generateMipMaps && scratchImage.GetMetadata().mipLevels == 1)
	{
		const DirectX::Image* origImg = scratchImage.GetImage(0, 0, 0);
		assert(origImg);
		hr = DirectX::GenerateMipMaps(
			*origImg,
			DirectX::TEX_FILTER_DEFAULT, // filter
			4, // levels
			processedImage);

		finalImage = &processedImage;
	} 
	else
	{
		finalImage = &scratchImage;
	}

	LOG_IF(WARNING, finalImage->GetImageCount() == 0) << "Image count is zero.";
	if (SUCCEEDED(hr)) {
		hr = DirectX::CreateShaderResourceView(device, finalImage->GetImages(), finalImage->GetImageCount(), metaData, &m_shaderResourceView);
	}

	return hr;
}

bool BufferTexture::load(ID3D11Device *device)
{
	return false;
}

bool FileTexture::load(ID3D11Device *device)
{
	HRESULT hr = S_OK;

	unload();

	if (isError)
		return false;

	DirectX::ScratchImage image;
	DirectX::TexMetadata metaData;
	if (str_ends(m_filename, L".dds"))
		hr = DirectX::LoadFromDDSFile(m_filename.c_str(), DirectX::DDS_FLAGS_NONE, &metaData, image);
	else if (str_ends(m_filename, L".tga"))
		hr = DirectX::LoadFromTGAFile(m_filename.c_str(), &metaData, image);
	else
		hr = DirectX::LoadFromWICFile(m_filename.c_str(), DirectX::WIC_FLAGS_NONE, &metaData, image);

	if (SUCCEEDED(hr))
		hr = loadShaderResourceView(device, metaData, image);

	if (SUCCEEDED(hr))
	{
		m_width = static_cast<uint32_t>(metaData.width);
		m_height = static_cast<uint32_t>(metaData.height);
	}

	if (FAILED(hr))
	{
		LOG(WARNING) << "Failed to create texture from file '" << utf8_encode(m_filename) << "'. " << to_string(hr);
		isError = true;
	}

	return !isError;
}

DepthTexture::DepthTexture(const DX::DeviceResources &deviceResources)
	: Texture(0, 0)
	, m_deviceResources(deviceResources)
{
}

bool DepthTexture::load(ID3D11Device *device)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = -1;

	auto depthStencilTexture = m_deviceResources.GetDepthStencil();
	return SUCCEEDED(device->CreateShaderResourceView(depthStencilTexture, &sr_desc, m_shaderResourceView.ReleaseAndGetAddressOf()));
}