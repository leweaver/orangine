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
}