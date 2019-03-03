﻿#pragma once
#include "IMaterial_manager.h"
#include "Renderer_enum.h"
#include "Renderer_data.h"
#include "Material_context.h"

namespace oe {
    class Texture;
    struct Renderer_animation_data;

    class Material_manager : public IMaterial_manager {
    public:

        explicit Material_manager(Scene& scene);

        Material_manager(const Material_manager& other) = delete;
        Material_manager(Material_manager&& other) = delete;
        void operator=(const Material_manager& other) = delete;
        void operator=(Material_manager&& other) = delete;

        // Manager_base implementation
        void initialize() override {}
        void shutdown() override {}

        // Manager_tickable implementation
        void tick() override;

        // Manager_deviceDependent implementation
        void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
        void destroyDeviceDependentResources() override;

        // IMaterial_manager implementation
        void bind(
            Material_context& materialContext,
            std::shared_ptr<const Material> material,
            const Mesh_vertex_layout& meshVertexLayout,
            const Render_light_data& renderLightData,
            Render_pass_blend_mode blendMode,
            bool enablePixelShader) override;

        void render(
            const Renderer_data& rendererData,
            const DirectX::SimpleMath::Matrix& worldMatrix,
            const Renderer_animation_data& rendererAnimationState,
            const Render_pass::Camera_data& camera) override;

        void unbind() override;

    protected:

        struct Material_constants {
            std::shared_ptr<D3D_buffer> vertexConstantBuffer;
            std::shared_ptr<D3D_buffer> pixelConstantBuffer;
        };
        // Indexed by Material::materialTypeIndex()
        std::vector<Material_constants> _materialConstants;


        DX::DeviceResources& deviceResources() const;

        void ensureMaterialConstants(const Material& material, Material_constants*& materialConstants);
        bool ensureSamplerState(
            Texture& texture,
            D3D11_TEXTURE_ADDRESS_MODE textureAddressMode,
            ID3D11SamplerState** d3D11SamplerState) const;

        void createVertexShader(Material_context::Compiled_material& compiledMaterial,
            const Material& material) const;
        void createPixelShader(Material_context::Compiled_material& compiledMaterial,
            const Material& material) const;

        std::shared_ptr<const Material> _boundMaterial;
        Render_pass_blend_mode _boundBlendMode;
        Microsoft::WRL::ComPtr<ID3D11Buffer> _boundLightDataConstantBuffer;
        std::shared_ptr<D3D_buffer> _boneTransformConstantBuffer;
        uint32_t _boundSrvCount = 0;
        uint32_t _boundSsCount = 0;
    };
}
