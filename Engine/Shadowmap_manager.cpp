﻿#include "pch.h"
#include "Shadowmap_manager.h"
#include "Shadow_map_texture_pool.h"
#include "Scene.h"

using namespace oe;
using namespace internal;

template<>
IShadowmap_manager* oe::create_manager(Scene& scene)
{
    return new Shadowmap_manager(scene);
}

void Shadowmap_manager::createDeviceDependentResources(DX::DeviceResources&)
{
	_texturePool = std::make_unique<Shadow_map_texture_pool>(256, 8);
    auto& deviceResources = _scene.manager<ID3D_resources_manager>().deviceResources();
	_texturePool->createDeviceDependentResources(deviceResources);
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

std::shared_ptr<Texture> Shadowmap_manager::shadowMapDepthTextureArray()
{
	verifyTexturePool();
	return _texturePool->shadowMapDepthTextureArray();
}

std::shared_ptr<Texture> Shadowmap_manager::shadowMapStencilTextureArray()
{
	verifyTexturePool();
	return _texturePool->shadowMapStencilTextureArray();
}

void Shadowmap_manager::verifyTexturePool() const
{
	if (!_texturePool) {
		throw std::logic_error("Invalid attempt to call shadowmap methods when no device is available");
	}
}
