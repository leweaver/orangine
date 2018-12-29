#pragma once

#include "Manager_base.h"
#include "Shadow_map_texture_pool.h"
#include <memory>

namespace oe {
	class Texture;
	class Shadow_map_texture;
	class Shadow_map_texture_array_slice;

	class IShadowmap_manager :
		public Manager_base,
		public Manager_deviceDependent
	{
	public:
		IShadowmap_manager(Scene& scene) : Manager_base(scene) {}
		virtual std::unique_ptr<Shadow_map_texture_array_slice> borrowTexture() = 0;
		virtual void returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap) = 0;
		virtual std::shared_ptr<Texture> shadowMapDepthTextureArray() = 0;
		virtual std::shared_ptr<Texture> shadowMapStencilTextureArray() = 0;
	};

	class Shadowmap_manager : public IShadowmap_manager {
	public:
		Shadowmap_manager(Scene& scene, DX::DeviceResources& deviceResources) 
			: IShadowmap_manager(scene)
			, _deviceResources(deviceResources)
			, _texturePool(nullptr)
		{}
		virtual ~Shadowmap_manager();

		// Manager_base implementation
		void initialize() override {}
		void shutdown() override {}

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		// IShadowmap_manager implementation
		std::unique_ptr<Shadow_map_texture_array_slice> borrowTexture() override;
		void returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap) override;
		std::shared_ptr<Texture> shadowMapDepthTextureArray() override;
		std::shared_ptr<Texture> shadowMapStencilTextureArray() override;

	private:
		void verifyTexturePool() const;

		DX::DeviceResources& _deviceResources;
		std::unique_ptr<Shadow_map_texture_pool> _texturePool;
	};
}