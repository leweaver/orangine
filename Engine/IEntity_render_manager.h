﻿#pragma once

#include "Manager_base.h"
#include "Primitive_mesh_data_factory.h"
#include "Deferred_light_material.h"
#include "Render_pass.h"
#include "Render_pass_config.h"
#include "Collision.h"
#include "Renderable.h"

#include <memory>
#include "Light_provider.h"

namespace oe {
    class Unlit_material;
    class Camera_component;
    class Renderable_component;
    class Scene;
    class Material;
    class Entity_filter;
    class Render_target_texture;
    class Entity_alpha_sorter;
    class Entity_cull_sorter;
    class Shadow_map_texture_pool;

    class IEntity_render_manager :
        public Manager_base,
        public Manager_tickable,
        public Manager_deviceDependent,
        public Manager_windowDependent
    {
    public:

        IEntity_render_manager(Scene& scene) : Manager_base(scene) {}

        virtual BoundingFrustumRH createFrustum(const Camera_component& cameraComponent) = 0;

        virtual void renderRenderable(Renderable& renderable,
            const DirectX::SimpleMath::Matrix& worldMatrix,
            float radius,
            const Render_pass::Camera_data& cameraData,
            const Light_provider::Callback_type& lightDataProvider,
            Render_pass_blend_mode blendMode,
            bool wireFrame
        ) = 0;

        virtual void renderEntity(Renderable_component& renderable,
            const Render_pass::Camera_data& cameraData,
            const Light_provider::Callback_type& lightDataProvider,
            Render_pass_blend_mode blendMode
        ) = 0;

        virtual Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) const = 0;
        virtual void clearRenderStats() = 0;
    };
}