#pragma once

#include "OeCore/IShadowmap_manager.h"
#include "OeCore/Shadow_map_texture_pool.h"
#include <memory>

namespace oe::internal {
	class Shadowmap_manager : public IShadowmap_manager {
	public:
		Shadowmap_manager(Scene& scene) 
			: IShadowmap_manager(scene)
			, _texturePool(nullptr)
		{}
        virtual ~Shadowmap_manager() = default;

		// Manager_base implementation
		void initialize() override {}
		void shutdown() override {}
        const std::string& name() const override;

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		// IShadowmap_manager implementation
		std::unique_ptr<Shadow_map_texture_array_slice> borrowTexture() override;
		void returnTexture(std::unique_ptr<Shadow_map_texture> shadowMap) override;
		std::shared_ptr<Texture> shadowMapDepthTextureArray() override;
		std::shared_ptr<Texture> shadowMapStencilTextureArray() override;

	private:

        static std::string _name;

		void verifyTexturePool() const;
		std::unique_ptr<Shadow_map_texture_pool> _texturePool;
	};
}