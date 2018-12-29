#include "pch.h"
#include "Shadow_map_texture_pool.h"

using namespace oe;

class Shadow_map_texture_array_texture : public Texture {
public:
	Shadow_map_texture_array_texture(ID3D11ShaderResourceView* srv)
		: Texture(srv)
	{
	}

	virtual void load(ID3D11Device* device)
	{
	}
};

Shadow_map_texture_pool::Shadow_map_texture_pool(uint32_t maxDimension, uint32_t textureArraySize)
	: _dimension(maxDimension)
	, _textureArraySize(textureArraySize)
{
	assert(is_power_of_two(maxDimension));
}

Shadow_map_texture_pool::~Shadow_map_texture_pool()
{
	if (_shadowMapArrayTexture2D) {
		LOG(WARNING) << "Shadow_map_texture_pool: Should call destroyDeviceDependentResources prior to object deletion";
		destroyDeviceDependentResources();
	}
}

void Shadow_map_texture_pool::createDeviceDependentResources(DX::DeviceResources& deviceResources)
{
	auto device = deviceResources.GetD3DDevice();

	// Initialize the render target texture description.
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = _dimension;
	textureDesc.Height = _dimension;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = _textureArraySize;
	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; 
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	LOG(INFO) << "Creating Shadow_map_texture_pool texture2D array with ArraySize=" << _textureArraySize <<
		", Width/Height=" << _dimension;

	ThrowIfFailed(device->CreateTexture2D(&textureDesc, nullptr, &_shadowMapArrayTexture2D),
		"Creating Shadow_map_texture_pool texture2D array");

	// Shader resource view for the array texture, which is shared by all of the slices.
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = _textureArraySize == 1 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	shaderResourceViewDesc.Texture2DArray.ArraySize = _textureArraySize;
	shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
	shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthShaderResourceView;
	ThrowIfFailed(device->CreateShaderResourceView(_shadowMapArrayTexture2D.Get(), &shaderResourceViewDesc, &depthShaderResourceView),
		"Creating Shadow_map_texture_pool depth shaderResourceView");

	shaderResourceViewDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stencilShaderResourceView;
	ThrowIfFailed(device->CreateShaderResourceView(_shadowMapArrayTexture2D.Get(), &shaderResourceViewDesc, &stencilShaderResourceView),
		"Creating Shadow_map_texture_pool stencil shaderResourceView");

	// Create the shadow maps
	_shadowMapDepthArrayTexture = std::make_shared<Shadow_map_texture_array_texture>(depthShaderResourceView.Get());
	_shadowMapStencilArrayTexture = std::make_shared<Shadow_map_texture_array_texture>(stencilShaderResourceView.Get());
	auto arrayTextureRetriever = [this, depthShaderResourceView]() {
		return Shadow_map_texture_array_slice::Array_texture {
			_shadowMapArrayTexture2D.Get(),
			depthShaderResourceView.Get(),
			_textureArraySize
		};
	};
	D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		static_cast<float>(_dimension),
		static_cast<float>(_dimension)
	);

	for (uint32_t slice = 0; slice < _textureArraySize; ++slice) {
		auto shadowMap = std::make_unique<Shadow_map_texture_array_slice>(viewport, _textureArraySize - slice - 1, arrayTextureRetriever);
		_shadowMaps.push_back(move(shadowMap));
	}
}

void Shadow_map_texture_pool::destroyDeviceDependentResources()
{
	if (_shadowMaps.size() != _textureArraySize)
		throw std::logic_error("Must return all borrowed textures prior to a call to destroyDeviceDependentResources");

	_shadowMaps.resize(0);
	_shadowMapDepthArrayTexture->unload();
	_shadowMapDepthArrayTexture.reset();
	_shadowMapStencilArrayTexture->unload();
	_shadowMapStencilArrayTexture.reset();
	_shadowMapArrayTexture2D.Reset();
}

std::unique_ptr<Shadow_map_texture_array_slice>
Shadow_map_texture_pool::borrowTexture()
{
	if (_shadowMaps.empty())
		throw std::runtime_error("shadow map pool is exhausted");

	auto shadowMap = move(_shadowMaps.back());
	_shadowMaps.pop_back();

	return shadowMap;
}

void Shadow_map_texture_pool::returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap)
{
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	shadowMap->getShaderResourceView()->GetResource(&resource);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
	ThrowIfFailed(resource.As<ID3D11Texture2D>(&texture2d));

	if (texture2d.Get() != _shadowMapArrayTexture2D.Get())
		throw std::logic_error("Attempt to return a shadowMapTexture that doesn't belong to this pool!");

	const auto arraySliceTexture = dynamic_cast<Shadow_map_texture_array_slice*>(shadowMap.get());
	assert(arraySliceTexture != nullptr);

	shadowMap.release();
	_shadowMaps.push_back(std::unique_ptr<Shadow_map_texture_array_slice>(arraySliceTexture));
}
