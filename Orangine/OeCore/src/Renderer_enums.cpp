// GENERATED BY codegen-enum.py
// Do not modify manually!

#include "pch.h"
#include "OeCore/EngineUtils.h"
#include "OeCore/Renderer_enums.h"

namespace oe {

///////////////////////////////////
// Material_alpha_mode
//
std::string g_materialAlphaModeNames[] = {
    "Opaque",
    "Mask",
    "Blend"
};
static_assert(static_cast<size_t>(Material_alpha_mode::Num_material_alpha_mode) == array_size(g_materialAlphaModeNames));
const std::string& materialAlphaModeToString(Material_alpha_mode enumValue)
{
    return g_materialAlphaModeNames[static_cast<size_t>(enumValue)];
}
Material_alpha_mode stringToMaterialAlphaMode(const std::string & str)
{
    return stringToEnum<Material_alpha_mode>(str, g_materialAlphaModeNames);
}

///////////////////////////////////
// Material_face_cull_mode
//
std::string g_materialFaceCullModeNames[] = {
    "Back_face",
    "Front_face",
    "None"
};
static_assert(static_cast<size_t>(Material_face_cull_mode::Num_material_face_cull_mode) == array_size(g_materialFaceCullModeNames));
const std::string& materialFaceCullModeToString(Material_face_cull_mode enumValue)
{
    return g_materialFaceCullModeNames[static_cast<size_t>(enumValue)];
}
Material_face_cull_mode stringToMaterialFaceCullMode(const std::string & str)
{
    return stringToEnum<Material_face_cull_mode>(str, g_materialFaceCullModeNames);
}

///////////////////////////////////
// Material_light_mode
//
std::string g_materialLightModeNames[] = {
    "Unlit",
    "Lit"
};
static_assert(static_cast<size_t>(Material_light_mode::Num_material_light_mode) == array_size(g_materialLightModeNames));
const std::string& materialLightModeToString(Material_light_mode enumValue)
{
    return g_materialLightModeNames[static_cast<size_t>(enumValue)];
}
Material_light_mode stringToMaterialLightMode(const std::string & str)
{
    return stringToEnum<Material_light_mode>(str, g_materialLightModeNames);
}

///////////////////////////////////
// Render_pass_blend_mode
//
std::string g_renderPassBlendModeNames[] = {
    "Opaque",
    "Blended_alpha",
    "Additive"
};
static_assert(static_cast<size_t>(Render_pass_blend_mode::Num_render_pass_blend_mode) == array_size(g_renderPassBlendModeNames));
const std::string& renderPassBlendModeToString(Render_pass_blend_mode enumValue)
{
    return g_renderPassBlendModeNames[static_cast<size_t>(enumValue)];
}
Render_pass_blend_mode stringToRenderPassBlendMode(const std::string & str)
{
    return stringToEnum<Render_pass_blend_mode>(str, g_renderPassBlendModeNames);
}

///////////////////////////////////
// Render_pass_depth_mode
//
std::string g_renderPassDepthModeNames[] = {
    "Read_write",
    "Read_only",
    "Write_only",
    "Disabled"
};
static_assert(static_cast<size_t>(Render_pass_depth_mode::Num_render_pass_depth_mode) == array_size(g_renderPassDepthModeNames));
const std::string& renderPassDepthModeToString(Render_pass_depth_mode enumValue)
{
    return g_renderPassDepthModeNames[static_cast<size_t>(enumValue)];
}
Render_pass_depth_mode stringToRenderPassDepthMode(const std::string & str)
{
    return stringToEnum<Render_pass_depth_mode>(str, g_renderPassDepthModeNames);
}

///////////////////////////////////
// Render_pass_stencil_mode
//
std::string g_renderPassStencilModeNames[] = {
    "Enabled",
    "Disabled"
};
static_assert(static_cast<size_t>(Render_pass_stencil_mode::Num_render_pass_stencil_mode) == array_size(g_renderPassStencilModeNames));
const std::string& renderPassStencilModeToString(Render_pass_stencil_mode enumValue)
{
    return g_renderPassStencilModeNames[static_cast<size_t>(enumValue)];
}
Render_pass_stencil_mode stringToRenderPassStencilMode(const std::string & str)
{
    return stringToEnum<Render_pass_stencil_mode>(str, g_renderPassStencilModeNames);
}

///////////////////////////////////
// Render_pass_destination
//
std::string g_renderPassDestinationNames[] = {
    "Default",
    "Gbuffer",
    "Render_target_view"
};
static_assert(static_cast<size_t>(Render_pass_destination::Num_render_pass_destination) == array_size(g_renderPassDestinationNames));
const std::string& renderPassDestinationToString(Render_pass_destination enumValue)
{
    return g_renderPassDestinationNames[static_cast<size_t>(enumValue)];
}
Render_pass_destination stringToRenderPassDestination(const std::string & str)
{
    return stringToEnum<Render_pass_destination>(str, g_renderPassDestinationNames);
}

///////////////////////////////////
// Vertex_attribute
//
std::string g_vertexAttributeNames[] = {
    "Position",
    "Color",
    "Normal",
    "Tangent",
    "Bi_tangent",
    "Tex_coord",
    "Joints",
    "Weights"
};
static_assert(static_cast<size_t>(Vertex_attribute::Num_vertex_attribute) == array_size(g_vertexAttributeNames));
const std::string& vertexAttributeToString(Vertex_attribute enumValue)
{
    return g_vertexAttributeNames[static_cast<size_t>(enumValue)];
}
Vertex_attribute stringToVertexAttribute(const std::string & str)
{
    return stringToEnum<Vertex_attribute>(str, g_vertexAttributeNames);
}

///////////////////////////////////
// Mesh_index_type
//
std::string g_meshIndexTypeNames[] = {
    "Triangles",
    "Lines"
};
static_assert(static_cast<size_t>(Mesh_index_type::Num_mesh_index_type) == array_size(g_meshIndexTypeNames));
const std::string& meshIndexTypeToString(Mesh_index_type enumValue)
{
    return g_meshIndexTypeNames[static_cast<size_t>(enumValue)];
}
Mesh_index_type stringToMeshIndexType(const std::string & str)
{
    return stringToEnum<Mesh_index_type>(str, g_meshIndexTypeNames);
}

///////////////////////////////////
// Animation_type
//
std::string g_animationTypeNames[] = {
    "Translation",
    "Rotation",
    "Scale",
    "Morph"
};
static_assert(static_cast<size_t>(Animation_type::Num_animation_type) == array_size(g_animationTypeNames));
const std::string& animationTypeToString(Animation_type enumValue)
{
    return g_animationTypeNames[static_cast<size_t>(enumValue)];
}
Animation_type stringToAnimationType(const std::string & str)
{
    return stringToEnum<Animation_type>(str, g_animationTypeNames);
}

///////////////////////////////////
// Animation_interpolation
//
std::string g_animationInterpolationNames[] = {
    "Linear",
    "Step",
    "Cubic_spline"
};
static_assert(static_cast<size_t>(Animation_interpolation::Num_animation_interpolation) == array_size(g_animationInterpolationNames));
const std::string& animationInterpolationToString(Animation_interpolation enumValue)
{
    return g_animationInterpolationNames[static_cast<size_t>(enumValue)];
}
Animation_interpolation stringToAnimationInterpolation(const std::string & str)
{
    return stringToEnum<Animation_interpolation>(str, g_animationInterpolationNames);
}

///////////////////////////////////
// Element_type
//
std::string g_elementTypeNames[] = {
    "Scalar",
    "Vector2",
    "Vector3",
    "Vector4",
    "Matrix2",
    "Matrix3",
    "Matrix4"
};
static_assert(static_cast<size_t>(Element_type::Num_element_type) == array_size(g_elementTypeNames));
const std::string& elementTypeToString(Element_type enumValue)
{
    return g_elementTypeNames[static_cast<size_t>(enumValue)];
}
Element_type stringToElementType(const std::string & str)
{
    return stringToEnum<Element_type>(str, g_elementTypeNames);
}

///////////////////////////////////
// Element_component
//
std::string g_elementComponentNames[] = {
    "Signed_byte",
    "Unsigned_byte",
    "Signed_short",
    "Unsigned_short",
    "Signed_int",
    "Unsigned_int",
    "Float"
};
static_assert(static_cast<size_t>(Element_component::Num_element_component) == array_size(g_elementComponentNames));
const std::string& elementComponentToString(Element_component enumValue)
{
    return g_elementComponentNames[static_cast<size_t>(enumValue)];
}
Element_component stringToElementComponent(const std::string & str)
{
    return stringToEnum<Element_component>(str, g_elementComponentNames);
}

///////////////////////////////////
// Debug_display_mode
//
std::string g_debugDisplayModeNames[] = {
    "None",
    "Normals",
    "World_positions",
    "Lighting"
};
static_assert(static_cast<size_t>(Debug_display_mode::Num_debug_display_mode) == array_size(g_debugDisplayModeNames));
const std::string& debugDisplayModeToString(Debug_display_mode enumValue)
{
    return g_debugDisplayModeNames[static_cast<size_t>(enumValue)];
}
Debug_display_mode stringToDebugDisplayMode(const std::string & str)
{
    return stringToEnum<Debug_display_mode>(str, g_debugDisplayModeNames);
}

///////////////////////////////////
// Sampler_filter_type
//
std::string g_samplerFilterTypeNames[] = {
    "Point",
    "Linear",
    "Point_mipmap_point",
    "Point_mipmap_linear",
    "Linear_mipmap_point",
    "Linear_mipmap_linear"
};
static_assert(static_cast<size_t>(Sampler_filter_type::Num_sampler_filter_type) == array_size(g_samplerFilterTypeNames));
const std::string& samplerFilterTypeToString(Sampler_filter_type enumValue)
{
    return g_samplerFilterTypeNames[static_cast<size_t>(enumValue)];
}
Sampler_filter_type stringToSamplerFilterType(const std::string & str)
{
    return stringToEnum<Sampler_filter_type>(str, g_samplerFilterTypeNames);
}

///////////////////////////////////
// Sampler_texture_address_mode
//
std::string g_samplerTextureAddressModeNames[] = {
    "Wrap",
    "Mirror",
    "Clamp",
    "Border",
    "Mirror_once"
};
static_assert(static_cast<size_t>(Sampler_texture_address_mode::Num_sampler_texture_address_mode) == array_size(g_samplerTextureAddressModeNames));
const std::string& samplerTextureAddressModeToString(Sampler_texture_address_mode enumValue)
{
    return g_samplerTextureAddressModeNames[static_cast<size_t>(enumValue)];
}
Sampler_texture_address_mode stringToSamplerTextureAddressMode(const std::string & str)
{
    return stringToEnum<Sampler_texture_address_mode>(str, g_samplerTextureAddressModeNames);
}

///////////////////////////////////
// Sampler_comparison_func
//
std::string g_samplerComparisonFuncNames[] = {
    "Never",
    "Less",
    "Equal",
    "Less_equal",
    "Greater",
    "Not_equal",
    "Greater_equal",
    "Always"
};
static_assert(static_cast<size_t>(Sampler_comparison_func::Num_sampler_comparison_func) == array_size(g_samplerComparisonFuncNames));
const std::string& samplerComparisonFuncToString(Sampler_comparison_func enumValue)
{
    return g_samplerComparisonFuncNames[static_cast<size_t>(enumValue)];
}
Sampler_comparison_func stringToSamplerComparisonFunc(const std::string & str)
{
    return stringToEnum<Sampler_comparison_func>(str, g_samplerComparisonFuncNames);
}

} // namespace oe