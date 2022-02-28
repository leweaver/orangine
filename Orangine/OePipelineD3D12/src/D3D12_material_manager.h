#pragma once

#include "D3D12_device_resources.h"

#include <OeCore/Material_manager.h>
#include <OeCore/Material_context.h>

namespace oe::pipeline_d3d12 {

class Material_context_impl;

class D3D12_material_manager : public Material_manager {
 public:
    D3D12_material_manager(IAsset_manager& assetManager, ITexture_manager& textureManager,
                         ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources);
    ~D3D12_material_manager() override = default;

    D3D12_material_manager(const D3D12_material_manager& other) = delete;
    D3D12_material_manager(D3D12_material_manager&& other) = delete;
    void operator=(const D3D12_material_manager& other) = delete;
    void operator=(D3D12_material_manager&& other) = delete;

    // Manager_deviceDependent implementation
    void createDeviceDependentResources() override;
    void destroyDeviceDependentResources() override;

    // IMaterial_manager implementation
    std::weak_ptr<Material_context> createMaterialContext() override;

    void createVertexShader(
            bool enableOptimizations,
            const Material& material,
            Material_context& materialContext) const override;
    void createPixelShader(
            bool enableOptimizations,
            const Material& material,
            Material_context& materialContext) const override;
    void createMaterialConstants(const Material& material) override {}
    void loadShaderResourcesToContext(
            const Material::Shader_resources& shaderResources,
            Material_context& materialContext) override;

    void bindLightDataToDevice(const Render_light_data* renderLightData) override {}
    void bindMaterialContextToDevice(const Material_context& materialContext, bool enablePixelShader)
            override {}
    void render(
            const Renderer_data& rendererData,
            const SSE::Matrix4& worldMatrix,
            const Renderer_animation_data& rendererAnimationData,
            const Camera_data& camera) override {}
    void unbind() override {}

    void updateLightBuffers() override {}

   private:

    Material_context_impl& getMaterialContextImpl(Material_context& context) const;

    static D3D12_PRIMITIVE_TOPOLOGY_TYPE getMeshDataTopology(Mesh_index_type meshIndexType);

    void compileShader(const Material::Shader_compile_settings& compileSettings, ID3DBlob** result) const;

    IAsset_manager& _assetManager;
    ITexture_manager& _textureManager;
    ILighting_manager& _lightingManager;
    IPrimitive_mesh_data_factory& _primitiveMeshDataFactory;
    D3D12_device_resources& _deviceResources;

    struct {
      Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
      std::vector<std::shared_ptr<Material_context_impl>> materialContexts;
      ID3D12Device6* device;
    } _deviceDependent;
};
}