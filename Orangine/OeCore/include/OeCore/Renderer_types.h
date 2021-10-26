#pragma once

#include "Collision.h"
#include "Renderer_enums.h"

#include <string>
#include <vectormath.hpp>

namespace oe {

// can be passed to some structs to invoke the explicit constructor that initializes default values.
struct Default_values {};

inline const uint32_t g_max_bone_transforms = 96;

struct Vertex_attribute_semantic {
  Vertex_attribute attribute;
  uint8_t semanticIndex;

  bool operator<(const Vertex_attribute_semantic& other) const {
    if (attribute == other.attribute)
      return semanticIndex < other.semanticIndex;
    return attribute < other.attribute;
  }
  bool operator==(const Vertex_attribute_semantic& other) const {
    return attribute == other.attribute && semanticIndex == other.semanticIndex;
  }
  bool operator!=(const Vertex_attribute_semantic& other) const { return !(*this == other); }
};

struct Vertex_attribute_element {
  Vertex_attribute_semantic semantic;
  Element_type type;
  Element_component component;
};

struct Sampler_descriptor {
  Sampler_filter_type minFilter;
  Sampler_filter_type magFilter;
  Sampler_texture_address_mode wrapU;
  Sampler_texture_address_mode wrapV;
  Sampler_texture_address_mode wrapW;
  Sampler_comparison_func comparisonFunc;

  Sampler_descriptor() = default;
  explicit Sampler_descriptor(Default_values)
      : minFilter(Sampler_filter_type::Point)
      , magFilter(Sampler_filter_type::Point)
      , wrapU(Sampler_texture_address_mode::Wrap)
      , wrapV(Sampler_texture_address_mode::Wrap)
      , wrapW(Sampler_texture_address_mode::Wrap)
      , comparisonFunc(Sampler_comparison_func::Never) {}
};

struct Viewport {
  float topLeftX;
  float topLeftY;
  float width;
  float height;
  float minDepth;
  float maxDepth;
};

struct Vector2u {
  uint32_t x;
  uint32_t y;
};

struct Vector2i {
  int32_t x;
  int32_t y;
};

// SIMPLE VECTOR TYPES
// For use in buffers, where it is imperative that it is exactly N floats big
// and packing order is important.
// Convert to a SSE::VectorN before performing mathematical operations.
struct Float2 {
  Float2() : x(0.0f), y(0.0f) {}
  Float2(float x, float y) : x(x), y(y) {}
  explicit Float2(const SSE::Vector3& vec) : x(vec.getX()), y(vec.getY()) {}

  float x, y;
};

struct Float3 {
  Float3() : x(0.0f), y(0.0f), z(0.0f) {}
  Float3(float x, float y, float z) : x(x), y(y), z(z) {}
  explicit Float3(const SSE::Vector3& vec) : x(vec.getX()), y(vec.getY()), z(vec.getZ()) {}

  SSE::Vector3 toVector3() const { return {x, y, z}; }

  float x, y, z;
};

struct Float4 {
  Float4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  Float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  Float4(const SSE::Vector3& v, float w) : x(v.getX()), y(v.getY()), z(v.getZ()), w(w) {}
  explicit Float4(const SSE::Vector4& vec)
      : x(vec.getX()), y(vec.getY()), z(vec.getZ()), w(vec.getW()) {}

  SSE::Vector4 toVector4() const { return {x, y, z, w}; }
  SSE::Quat toQuaternion() const { return {x, y, z, w}; }

  float x, y, z, w;
};

struct Camera_data {
  SSE::Matrix4 viewMatrix;
  SSE::Matrix4 projectionMatrix;
  float fov;
  float aspectRatio;
  bool enablePixelShader = true;

  static const Camera_data IDENTITY;
};

struct Depth_stencil_config {
  Depth_stencil_config() {}
  Depth_stencil_config(Render_pass_blend_mode blendMode, Render_pass_depth_mode depthMode)
      : Depth_stencil_config(Default_values()) {
    this->blendMode = blendMode;
    this->depthMode = depthMode;
  }
  explicit Depth_stencil_config(Default_values)
      : blendMode(Render_pass_blend_mode::Opaque)
      , depthMode(Render_pass_depth_mode::Read_write)
      , stencilMode(Render_pass_stencil_mode::Disabled)
      , stencilReadMask(0xff)
      , stencilWriteMask(0xff) {}
  Render_pass_blend_mode blendMode;
  Render_pass_depth_mode depthMode;
  Render_pass_stencil_mode stencilMode;
  uint32_t stencilReadMask;
  uint32_t stencilWriteMask;
};

} // namespace oe