#pragma once

#include "Collision.h"
#include "EngineUtils.h"
#include "Renderer_enums.h"

#include <string>
#include <vectormath.hpp>

#include <dxgiformat.h>
#include <gsl/span>

namespace oe {

// can be passed to some structs to invoke the explicit constructor that initializes default values.
struct Default_values {};

using Material_context_handle = int64_t;
inline static constexpr Material_context_handle kInvalidMaterialContext = 0;

inline const uint32_t g_max_bone_transforms = 96;

// Alignment size of 16 bits. Primarily used for shader constants
inline static constexpr size_t g_align_16 = 16;

struct Vertex_attribute_semantic {
  Vertex_attribute attribute;
  uint8_t semanticIndex;

  bool operator<(const Vertex_attribute_semantic& other) const
  {
    if (attribute == other.attribute) return semanticIndex < other.semanticIndex;
    return attribute < other.attribute;
  }
  bool operator==(const Vertex_attribute_semantic& other) const
  {
    return attribute == other.attribute && semanticIndex == other.semanticIndex;
  }
  bool operator!=(const Vertex_attribute_semantic& other) const { return !(*this == other); }
};

struct Vertex_attribute_element {
  Vertex_attribute_element() = delete;
  Vertex_attribute_element(Vertex_attribute_semantic semantic, Element_type type, Element_component component)
      : semantic(semantic)
      , type(type)
      , component(component)
  {}
  Vertex_attribute_element(const Vertex_attribute_element& other)
      : Vertex_attribute_element(other.semantic, other.type, other.component)
  {}
  const Vertex_attribute_semantic semantic;
  const Element_type type;
  const Element_component component;
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
      , comparisonFunc(Sampler_comparison_func::Never)
  {}

  bool operator==(const Sampler_descriptor& other) const
  {
    return minFilter == other.minFilter && magFilter == other.magFilter && wrapU == other.wrapU &&
           wrapV == other.wrapV && wrapW == other.wrapW && comparisonFunc == other.comparisonFunc;
  }

  bool operator!=(const Sampler_descriptor& other) const { return !(*this == other); }
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
  Float2()
      : x(0.0f)
      , y(0.0f)
  {}
  Float2(float x, float y)
      : x(x)
      , y(y)
  {}
  explicit Float2(const SSE::Vector3& vec)
      : x(vec.getX())
      , y(vec.getY())
  {}

  float x, y;
};

struct Float3 {
  Float3()
      : x(0.0f)
      , y(0.0f)
      , z(0.0f)
  {}
  Float3(float x, float y, float z)
      : x(x)
      , y(y)
      , z(z)
  {}
  explicit Float3(const SSE::Vector3& vec)
      : x(vec.getX())
      , y(vec.getY())
      , z(vec.getZ())
  {}

  SSE::Vector3 toVector3() const { return {x, y, z}; }

  float x, y, z;
};

struct Float4 {
  Float4()
      : x(0.0f)
      , y(0.0f)
      , z(0.0f)
      , w(0.0f)
  {}
  Float4(float x, float y, float z, float w)
      : x(x)
      , y(y)
      , z(z)
      , w(w)
  {}
  Float4(const SSE::Vector3& v, float w)
      : x(v.getX())
      , y(v.getY())
      , z(v.getZ())
      , w(w)
  {}
  explicit Float4(const SSE::Vector4& vec)
      : x(vec.getX())
      , y(vec.getY())
      , z(vec.getZ())
      , w(vec.getW())
  {}

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
  explicit Depth_stencil_config(
          Render_pass_blend_mode blendMode = Render_pass_blend_mode::Opaque,
          Render_pass_depth_mode depthMode = Render_pass_depth_mode::Read_write,
          Render_pass_stencil_mode stencilMode = Render_pass_stencil_mode::Disabled)
      : _blendMode(blendMode)
      , _depthMode(depthMode)
      , _stencilMode(stencilMode)
      , _stencilReadMask(0xff)
      , _stencilWriteMask(0xff)
  {
    std::hash<int> hashGen;
    _modeHash = hashGen(static_cast<int>(blendMode)) ^ hashGen(static_cast<int>(depthMode)) ^
                hashGen(static_cast<int>(stencilMode));
  }

  inline Render_pass_blend_mode getBlendMode() const { return _blendMode; }
  inline Render_pass_depth_mode getDepthMode() const { return _depthMode; }
  inline Render_pass_stencil_mode getStencilMode() const { return _stencilMode; }
  inline uint32_t getStencilReadMask() const { return _stencilReadMask; }
  inline uint32_t getStencilWriteMask() const { return _stencilWriteMask; }
  size_t getModeHash() const { return _modeHash; }

  void setStencilReadMask(uint32_t stencilReadMask) { _stencilReadMask = stencilReadMask; }
  void setStencilWriteMask(uint32_t stencilWriteMask) { _stencilWriteMask = stencilWriteMask; }

 private:
  Render_pass_blend_mode _blendMode;
  Render_pass_depth_mode _depthMode;
  Render_pass_stencil_mode _stencilMode;
  uint32_t _stencilReadMask;
  uint32_t _stencilWriteMask;
  size_t _modeHash;
};

template<typename CTy, typename CTr>
std::basic_ostream<CTy, CTr>& operator<<(std::basic_ostream<CTy, CTr>& ss, const Depth_stencil_config& config)
{
  ss << "{ blendMode=" << renderPassBlendModeToString(config.getBlendMode()) << ", "
     << "depthMode=" << renderPassDepthModeToString(config.getDepthMode()) << ", "
     << "stencilMode=" << renderPassStencilModeToString(config.getStencilMode()) << " }";
  return ss;
}

using Shader_external_constant_buffer_handle = int64_t;

class Shader_layout_constant_buffer {
 public:
  struct Usage_per_draw_data {
    size_t bufferSizeInBytes;
  };
  struct Usage_external_buffer_data {
    Shader_external_constant_buffer_handle handle;
  };

  Shader_layout_constant_buffer()
      : Shader_layout_constant_buffer(
                0, Shader_constant_buffer_visibility::Num_shader_constant_buffer_visibility, Usage_per_draw_data{0})
  {}
  Shader_layout_constant_buffer(
          uint32_t registerIndex, Shader_constant_buffer_visibility visibility, Usage_per_draw_data data)
      : _registerIndex(registerIndex)
      , _usage(Shader_constant_buffer_usage::Per_draw)
      , _visibility(visibility)
      , _usagePerDraw(data)
  {}
  Shader_layout_constant_buffer(
          uint32_t registerIndex, Shader_constant_buffer_visibility visibility, Usage_external_buffer_data data)
      : _registerIndex(registerIndex)
      , _usage(Shader_constant_buffer_usage::External_buffer)
      , _visibility(visibility)
      , _usageExternalBuffer(data)
  {}

  const uint32_t getRegisterIndex() const { return _registerIndex; }
  const Shader_constant_buffer_usage getUsage() const { return _usage; }
  const Shader_constant_buffer_visibility getVisibility() const { return _visibility; }
  const Usage_per_draw_data& getUsagePerDraw() const
  {
    OE_CHECK(_usage == Shader_constant_buffer_usage::Per_draw);
    return _usagePerDraw;
  }
  const Usage_external_buffer_data& getUsageExternalBuffer() const
  {
    OE_CHECK(_usage == Shader_constant_buffer_usage::External_buffer);
    return _usageExternalBuffer;
  }

 private:
  const uint32_t _registerIndex;
  const Shader_constant_buffer_usage _usage;
  const Shader_constant_buffer_visibility _visibility;

  union {
    const Usage_per_draw_data _usagePerDraw;
    const Usage_external_buffer_data _usageExternalBuffer;
  };
};

struct Shader_constant_layout {
  gsl::span<const Shader_layout_constant_buffer> constantBuffers;
};

using Texture_format = DXGI_FORMAT;
struct Shader_output_layout {
  gsl::span<const Texture_format> renderTargetCountFormats;
};

}// namespace oe

namespace std {

template<> struct hash<oe::Vertex_attribute_semantic> {
  std::size_t operator()(const oe::Vertex_attribute_semantic& k) const
  {
    using std::hash;
    return ((hash<uint8_t>()(static_cast<uint8_t>(k.attribute)) ^ (hash<uint8_t>()(k.semanticIndex) << 1)) >> 1);
  }
};
}// namespace std