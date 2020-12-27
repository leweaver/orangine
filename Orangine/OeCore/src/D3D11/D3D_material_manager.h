#pragma once

#include "../Material_manager.h"

#include "OeCore/Material_context.h"

#include "D3D_device_repository.h"
#include "D3D_Renderer_data.h"


#include <vector>

struct ID3D11Device;

namespace oe {

class Scene;
class Material;
struct D3D_buffer;

class D3D_material_context : public Material_context {
 public:
  ~D3D_material_context() {
    resetShaderResourceViews();
    resetSamplerStates();
  }

  void reset() override;

  void resetShaderResourceViews() {
    for (auto srv : shaderResourceViews) {
      if (srv)
        srv->Release();
    }
    shaderResourceViews.resize(0);
  }
  void resetSamplerStates() {
    for (auto ss : samplerStates) {
      if (ss)
        ss->Release();
    }
    samplerStates.resize(0);
  }

  // These are not stored as ComPtr, so that the array can be passed directly to d3d.
  // When adding and removing from here, remember to AddRef and Release.
  std::vector<ID3D11ShaderResourceView*> shaderResourceViews;
  std::vector<ID3D11SamplerState*> samplerStates;

  // D3D Shaders
  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};

class D3D_material_manager : public Material_manager {
 public:
  D3D_material_manager(
      Scene& scene,
      std::shared_ptr<D3D_device_repository> deviceRepository);
  ~D3D_material_manager() = default;

  D3D_material_manager(const D3D_material_manager& other) = delete;
  D3D_material_manager(D3D_material_manager&& other) = delete;
  void operator=(const D3D_material_manager& other) = delete;
  void operator=(D3D_material_manager&& other) = delete;

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
  void createMaterialConstants(const Material& material) override;
  void loadShaderResourcesToContext(
      const Material::Shader_resources& shaderResources,
      Material_context& materialContext) override;

  void bindLightDataToDevice(const Render_light_data* renderLightData) override;
  void bindMaterialContextToDevice(const Material_context& materialContext, bool enablePixelShader)
      override;
  void render(
      const Renderer_data& rendererData,
      const SSE::Matrix4& worldMatrix,
      const Renderer_animation_data& rendererAnimationData,
      const Camera_data& camera) override;
  void unbind() override;

  // The template arguments here must match the size of the lights array in the shader constant
  // buffer files.
  Render_light_data_impl<8>* getRenderLightDataLit() override { return _renderLightData_lit.get(); }
  Render_light_data_impl<0>* getRenderLightDataUnlit() override {
    return _renderLightData_unlit.get();
  }

  void updateLightBuffers() override;

  static const D3D_material_context& verifyAsD3dMaterialContext(
      const Material_context& materialContext);
  static D3D_material_context& verifyAsD3dMaterialContext(Material_context& materialContext);

 private:
  DX::DeviceResources& deviceResources() const;

  std::shared_ptr<D3D_buffer> createVsConstantBuffer(
      const Material& material,
      ID3D11Device* device);
  std::shared_ptr<D3D_buffer> createPsConstantBuffer(
      const Material& material,
      ID3D11Device* device);

  void updateVsConstantBuffer(
      const Material& material,
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      const Renderer_animation_data& rendererAnimationData,
      ID3D11DeviceContext* context,
      D3D_buffer& buffer);

  void updatePsConstantBuffer(
      const Material& material,
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      ID3D11DeviceContext* context,
      D3D_buffer& buffer);

  struct Material_constants {
    std::shared_ptr<D3D_buffer> vertexConstantBuffer;
    std::shared_ptr<D3D_buffer> pixelConstantBuffer;
  };

  void ensureMaterialConstants(const Material& material, Material_constants*& materialConstants);

  D3D11_FILTER convertFilter(const Sampler_descriptor& descriptor, float maxAnisotropy);

  // Indexed by Material::materialTypeIndex()
  std::vector<Material_constants> _materialConstants;

  Microsoft::WRL::ComPtr<ID3D11Buffer> _boundLightDataConstantBuffer;
  std::shared_ptr<D3D_buffer> _boneTransformConstantBuffer;
  uint32_t _boundSrvCount = 0;
  uint32_t _boundSsCount = 0;

  std::array<
      D3D11_TEXTURE_ADDRESS_MODE,
      (int)Sampler_texture_address_mode::Num_sampler_texture_address_mode>
      _textureAddressModeLUT;
  std::array<D3D11_COMPARISON_FUNC, (int)Sampler_comparison_func::Num_sampler_comparison_func>
      _textureComparisonFuncLUT;

  std::array<D3D11_FILTER_TYPE, (int)Sampler_filter_type::Num_sampler_filter_type>
      _textureMinFilterMinLUT;
  std::array<D3D11_FILTER_TYPE, (int)Sampler_filter_type::Num_sampler_filter_type>
      _textureMinFilterMipLUT;
  std::array<D3D11_FILTER_TYPE, (int)Sampler_filter_type::Num_sampler_filter_type>
      _textureMagFilterMagLUT;

  std::shared_ptr<D3D_device_repository> _deviceRepository;
  std::vector<uint8_t> _bufferTemp;
  std::vector<std::shared_ptr<D3D_material_context>> _createdMaterialContexts;

  // The template arguments here must match the size of the lights array in the shader constant
  // buffer files.
  std::unique_ptr<D3D_render_light_data<0>> _renderLightData_unlit;
  std::unique_ptr<D3D_render_light_data<8>> _renderLightData_lit;
};
} // namespace oe