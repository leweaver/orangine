#pragma once

#include "Manager_base.h"
#include "Render_pass.h"
#include "Renderer_types.h"

#include <memory>
#include <array>

namespace oe {
    class Camera_component;
    class Deferred_light_material;
    class Render_light_data;
    class Unlit_material;
    class Entity_alpha_sorter;
    class Entity_cull_sorter;
    class Entity_filter;
    class Entity;

    class IRender_step_manager :
        public Manager_base,
        public Manager_deviceDependent,
        public Manager_windowDependent
    {
    public:
        explicit IRender_step_manager(Scene& scene)
            : Manager_base(scene)
        {}

        template<class TData, class... TRender_passes>
        class Render_step {
        public:
            explicit Render_step(std::shared_ptr<TData> data) : data(data) {}

            bool enabled = true;
            std::shared_ptr<TData> data;
            std::tuple<TRender_passes...> renderPassConfigs;
            std::array<std::unique_ptr<Render_pass>, sizeof...(TRender_passes)> renderPasses;
        };

        // IRender_step_manager
        virtual void createRenderSteps() = 0;
        virtual Render_pass::Camera_data createCameraData(Camera_component& component) const = 0;
        virtual Viewport screenViewport() const = 0;
        virtual void render(std::shared_ptr<Entity> cameraEntity) = 0;

    };
}
