#pragma once

#include "Manager_base.h"
#include "Primitive_mesh_data_factory.h"
#include "Deferred_light_material.h"
#include "Collision.h"

#include "Environment_volume.h"
#include "Light_provider.h"
#include <memory>

namespace oe {
    class Unlit_material;
    class Camera_component;
    class Renderable_component;
    class Scene;
    class Material;
    class Entity_filter;
    class Entity_alpha_sorter;
    class Entity_cull_sorter;
    class Shadow_map_texture_pool;
    struct Renderable;

    class IEntity_render_manager :
        public Manager_base,
        public Manager_deviceDependent
    {
    public:

        explicit IEntity_render_manager(Scene& scene) : Manager_base(scene) {}

        virtual void renderRenderable(Renderable& renderable,
            const SSE::Matrix4& worldMatrix,
            float radius,
            const Camera_data& cameraData,
            const Light_provider::Callback_type& lightDataProvider,
            Render_pass_blend_mode blendMode,
            bool wireFrame
        ) = 0;

        virtual void renderEntity(Renderable_component& renderable,
            const Camera_data& cameraData,
            const Light_provider::Callback_type& lightDataProvider,
            Render_pass_blend_mode blendMode
        ) = 0;

        virtual Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) = 0;
        virtual void clearRenderStats() = 0;
    };
}