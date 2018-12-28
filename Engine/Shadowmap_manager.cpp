#include "pch.h"
#include "Shadowmap_manager.h"
#include "Shadow_map_texture_pool.h"

using namespace oe;

Shadowmap_manager::~Shadowmap_manager()
{
	_texturePool.reset();
}

void Shadowmap_manager::createDeviceDependentResources(DX::DeviceResources& deviceResources)
{
	_texturePool = std::make_unique<Shadow_map_texture_pool>(256, 8);
	_texturePool->createDeviceDependentResources(_deviceResources);
}

void Shadowmap_manager::destroyDeviceDependentResources()
{
	_texturePool->destroyDeviceDependentResources();
	_texturePool.reset();
}

std::unique_ptr<Shadow_map_texture_array_slice> Shadowmap_manager::borrowTexture()
{
	verifyTexturePool();
	return _texturePool->borrowTexture();
}

void Shadowmap_manager::returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap)
{
	verifyTexturePool();
	_texturePool->returnTexture(move(shadowMap));
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapTextureArray()
{
	verifyTexturePool();
	return _texturePool->shadowMapTextureArray();
}

void Shadowmap_manager::verifyTexturePool()
{
	if (!_texturePool) {
		throw new std::logic_error("Invalid attempt to call shadowmap methods when no device is available");
	}
}
