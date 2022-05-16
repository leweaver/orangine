﻿#pragma once
#include "EngineUtils.h"
#include "Material.h"

namespace oe {

__declspec(align(g_align_16)) struct Vertex_constant_buffer_empty {};
__declspec(align(g_align_16)) struct Vertex_constant_buffer_base { SSE::Matrix4 worldViewProjection; };

__declspec(align(g_align_16)) struct Pixel_constant_buffer_empty {};
using Pixel_constant_buffer_base = Pixel_constant_buffer_empty;

template <class TVertex_shader_constants, class TPixel_shader_constants>
class Material_base : public Material {
 public:
  explicit Material_base(
      uint8_t materialTypeIndex,
      Material_alpha_mode alphaMode = Material_alpha_mode::Opaque,
      Material_face_cull_mode faceCullMode = Material_face_cull_mode::Back_face)
      : Material(materialTypeIndex, alphaMode, faceCullMode) {
    static_assert(
        std::is_same_v<TVertex_shader_constants, Vertex_constant_buffer_empty> ||
        sizeof(TVertex_shader_constants) % 16 == 0);
    static_assert(
        std::is_same_v<TPixel_shader_constants, Pixel_constant_buffer_empty> ||
        sizeof(TPixel_shader_constants) % 16 == 0);
  }

  void updateVsConstantBuffer(
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      const Renderer_animation_data& rendererAnimationData,
      uint8_t* buffer,
      size_t bufferSize) const override {
    size_t shaderConstantsSize =sizeof(TVertex_shader_constants);
    OE_CHECK(bufferSize >= shaderConstantsSize);
    auto& constantsVs = *reinterpret_cast<TVertex_shader_constants*>(buffer);

    if constexpr (std::is_assignable_v<Vertex_constant_buffer_base, TVertex_shader_constants>) {
      constantsVs.worldViewProjection = projMatrix * viewMatrix * worldMatrix;
    }
    updateVsConstantBufferValues(
        constantsVs, worldMatrix, viewMatrix, projMatrix, rendererAnimationData);
  }

  void updatePsConstantBuffer(
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      uint8_t* buffer,
      size_t bufferSize) const override {
    size_t shaderConstantsSize =sizeof(TPixel_shader_constants);
    OE_CHECK(bufferSize >= shaderConstantsSize);
    auto& constantsPs = *reinterpret_cast<TPixel_shader_constants*>(buffer);

    updatePsConstantBufferValues(constantsPs, worldMatrix, viewMatrix, projMatrix);
  };

  std::vector<Vertex_attribute_element> vertexInputs(
      const std::set<std::string>& flags) const override {
    std::vector<Vertex_attribute_element> vsInputs{
        {{Vertex_attribute::Position, 0}, Element_type::Vector3, Element_component::Float}};
    return vsInputs;
  }

  Shader_constant_layout getShaderConstantLayout() const override
  {
    static constexpr std::array<Shader_layout_constant_buffer, 2> baseConstantLayout{
            {{0, sizeof(TVertex_shader_constants), Shader_constant_buffer_usage::Per_draw,
              Shader_constant_buffer_visibility::Vertex},
             {1, sizeof(TPixel_shader_constants), Shader_constant_buffer_usage::Per_draw,
              Shader_constant_buffer_visibility::Pixel}}};
    return Shader_constant_layout{baseConstantLayout};
  }

  Shader_output_layout getShaderOutputLayout() const override
  {
    static constexpr std::array<Texture_format, 1> baseRtvFormats {{ DXGI_FORMAT_B8G8R8A8_UNORM }};
    return Shader_output_layout { baseRtvFormats };
  }

 protected:
  Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override {
    std::stringstream ss;
    ss << materialType() << "_VS.hlsl";
    auto settings = Material::vertexShaderSettings(flags);
    settings.filename = ss.str();
    return settings;
  }

  Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override {
    std::stringstream ss;
    ss << materialType() << "_PS.hlsl";
    auto settings = Material::pixelShaderSettings(flags);
    settings.filename = ss.str();
    return settings;
  }

  virtual void updateVsConstantBufferValues(
      TVertex_shader_constants& constants,
      const SSE::Matrix4& matrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix,
      const Renderer_animation_data& rendererAnimationData) const {};

  virtual void updatePsConstantBufferValues(
      TPixel_shader_constants& constants,
      const SSE::Matrix4& worldMatrix,
      const SSE::Matrix4& viewMatrix,
      const SSE::Matrix4& projMatrix) const {};
};
} // namespace oe
