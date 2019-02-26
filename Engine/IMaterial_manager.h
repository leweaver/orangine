#pragma once

#include "Manager_base.h"
#include "Renderer_enum.h"
#include "Render_pass.h"

namespace oe {
    class Material_context;
    struct Renderer_data;
    class Render_light_data;
    class Scene;
    class Material;
    class Mesh_vertex_layout;
    struct Renderer_animation_data;
    
    class IMaterial_manager :
        public Manager_base,
        public Manager_tickable
    {
    public:
        explicit IMaterial_manager(Scene& scene) : Manager_base(scene) {}
               
        // Sets the Material that is used for all subsequent calls to render.
        // Compiles (if needed), binds pixel and vertex shaders, and textures.
        virtual void bind(
            Material_context& materialContext,
            std::shared_ptr<const Material> material,
            const Mesh_vertex_layout& meshVertexLayout,
            const Render_light_data& renderLightData,
            Render_pass_blend_mode blendMode,
            bool enablePixelShader) = 0;
        
        // Uploads shader constants, then renders
        virtual void render(
            const Renderer_data& rendererData,
            const DirectX::SimpleMath::Matrix& worldMatrix,
            const Renderer_animation_data& rendererAnimationState,
            const Render_pass::Camera_data& camera) = 0;

        // Unbinds the current material. Must be called when rendering of an object is complete.
        virtual void unbind() = 0;
    };
}
