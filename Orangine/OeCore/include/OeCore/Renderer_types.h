#pragma once

#include "Collision.h"

#include <vectormath/vectormath.hpp>
#include <string>

namespace oe {

// can be passed to some structs to invoke the explicit constructor that initializes default values.
struct Default_values {};

inline const uint32_t g_max_bone_transforms = 96;

enum class Material_alpha_mode {
  Opaque = 0,
  Mask,
  Blend,

  Num_Alpha_Mode
};
const std::string& materialAlphaModeToString(Material_alpha_mode enumValue);
Material_alpha_mode stringToMaterialAlphaMode(const std::string& str);

enum class Material_face_cull_mode {
  Back_Face = 0,
  Front_Face,
  None,

  Num_Face_Cull_Mode
};
const std::string& materialFaceCullModeToString(Material_face_cull_mode enumValue);
Material_face_cull_mode stringToMaterialFaceCullMode(const std::string& str);

enum class Material_light_mode {
  Unlit = 0,
  Lit,

  Num_Light_Mode
};
const std::string& materialLightModeToString(Material_light_mode enumValue);
Material_light_mode stringToMaterialLightMode(const std::string& str);

enum class Render_pass_blend_mode {
  Opaque = 0,
  Blended_Alpha,
  Additive,

  Num_Blend_Mode
};
const std::string& renderPassBlendModeToString(Render_pass_blend_mode enumValue);
Render_pass_blend_mode stringToRenderPassBlendMode(const std::string& str);

enum class Render_pass_depth_mode {
  ReadWrite = 0,
  ReadOnly,
  WriteOnly,
  Disabled,

  Num_Depth_Mode
};
const std::string& renderPassDepthModeToString(Render_pass_depth_mode enumValue);
Render_pass_depth_mode stringToRenderPassDepthMode(const std::string& str);

enum class Render_pass_stencil_mode {
  Enabled = 0,
  Disabled,

  Num_Stencil_Mode
};
const std::string& renderPassStencilModeToString(Render_pass_stencil_mode enumValue);
Render_pass_stencil_mode stringToRenderPassStencilMode(const std::string& str);

enum class Vertex_attribute : std::int8_t {
  Position = 0,
  Color,
  Normal,
  Tangent,
  Bi_Tangent,
  Tex_Coord,
  Joints,
  Weights,

  Num_Vertex_Attribute,
};
const std::string& vertexAttributeToString(Vertex_attribute enumValue);
Vertex_attribute stringToVertexAttribute(const std::string& str);

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

enum class Mesh_index_type : std::int8_t {
  Triangles,
  Lines,

  Num_Mesh_Index_Type
};
const std::string& meshIndexTypeToString(Mesh_index_type enumValue);
Mesh_index_type stringToMeshIndexType(const std::string& str);

enum class Animation_type {
  Translation = 0,
  Rotation,
  Scale,
  Morph,

  Num_Animation_Type
};
const std::string& animationTypeToString(Animation_type enumValue);
Animation_type stringToAnimationType(const std::string& str);

enum class Animation_interpolation {
  Linear = 0,
  Step,
  Cubic_Spline,

  Num_Animation_Interpolation
};
const std::string& animationInterpolationToString(Animation_interpolation enumValue);
Animation_interpolation stringToAnimationInterpolation(const std::string& str);

enum class Element_type {
  Scalar = 0,
  Vector2,
  Vector3,
  Vector4,
  Matrix2,
  Matrix3,
  Matrix4,

  Num_Element_Type
};
const std::string& elementTypeToString(Element_type enumValue);
Element_type stringToElementType(const std::string& str);

enum class Element_component {
  Signed_Byte = 0,
  Unsigned_Byte,
  Signed_Short,
  Unsigned_Short,
  Signed_Int,
  Unsigned_Int,
  Float,

  Num_Element_Component
};
const std::string& elementComponentToString(Element_component enumValue);
Element_component stringToElementComponent(const std::string& str);

enum class Debug_display_mode {
  None = 0,
  Normals,
  World_Positions,
  Lighting,

  Num_Debug_Display_Mode
};
const std::string& debugDisplayModeToString(Debug_display_mode enumValue);
Debug_display_mode stringToDebugDisplayMode(const std::string& str);

enum class Sampler_filter_type {
  Point = 0,
  Linear,
  Point_mipmap_point,
  Point_mipmap_linear,
  Linear_mipmap_point,
  Linear_mipmap_linear,

  Num_sampler_filter_type,
};
const std::string& samplerFilterTypeToString(Sampler_filter_type enumValue);
Sampler_filter_type stringToSamplerFilterType(const std::string& str);

enum class Sampler_texture_address_mode {
  Wrap = 0,
  Mirror,
  Clamp,
  Border,
  Mirror_once,

  Num_sampler_texture_address_mode,
};
const std::string& samplerTextureAddressModeToString(Sampler_texture_address_mode enumValue);
Sampler_texture_address_mode stringToSamplerTextureAddressMode(const std::string& str);

enum class Sampler_comparison_func {
  Never,
  Less,
  Equal,
  Less_equal,
  Greater,
  Not_equal,
  Greater_equal,
  Always,

  Num_sampler_comparison_func,
};
const std::string& samplerComparisonFuncToString(Sampler_comparison_func enumValue);
Sampler_comparison_func stringToSamplerComparisonFunc(const std::string& str);

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

struct Uint2 {
  uint32_t x;
  uint32_t y;
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

} // namespace oe