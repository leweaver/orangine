#include "pch.h"
#include "OeCore/Renderer_enum.h"
#include "OeCore/EngineUtils.h"

using namespace oe;

template <typename TEnum, size_t TCount>
TEnum stringToEnum(const std::string& value, const std::string(&strings)[TCount])
{
    for (auto i = 0; i < TCount; i++) {
        if (strings[i] == value)
            return static_cast<TEnum>(i);
    }
    OE_THROW(std::runtime_error("Unknown enum value: " + value));
}

std::string g_alphaModeNames[] = {
    "opaque",
    "mask",
    "blend"
};
static_assert(static_cast<size_t>(Material_alpha_mode::Num_Alpha_Mode) == array_size(g_alphaModeNames));

std::string g_faceCullModeNames[] = {
    "back_face",
    "front_face",
    "none"
};
static_assert(static_cast<size_t>(Material_face_cull_mode::Num_Face_Cull_Mode) == array_size(g_faceCullModeNames));

std::string g_lightModeNames[] = {
    "unlit",
    "lit"
};
static_assert(static_cast<size_t>(Material_light_mode::Num_Light_Mode) == array_size(g_lightModeNames));

std::string g_blendModeNames[] = {
    "opaque",
    "blended_alpha",
    "additive"
};
static_assert(static_cast<size_t>(Render_pass_blend_mode::Num_Blend_Mode) == array_size(g_blendModeNames));

std::string g_depthModeNames[] = {
    "read_write",
    "read_only",
    "write_only",
    "disabled"
};
static_assert(static_cast<size_t>(Render_pass_depth_mode::Num_Depth_Mode) == array_size(g_depthModeNames));

std::string g_stencilModeNames[] = {
    "enabled",
    "disabled"
};
static_assert(static_cast<size_t>(Render_pass_stencil_mode::Num_Stencil_Mode) == array_size(g_stencilModeNames));

std::string g_vertexAttributeNames[] = {
    "position",
    "color",
    "normal",
    "tangent",
    "bi_tangent",
    "tex_coord",
    "joints",
    "weights",
};
static_assert(static_cast<size_t>(Vertex_attribute::Num_Vertex_Attribute) == array_size(g_vertexAttributeNames));

std::string g_meshIndexTypeNames[] = {
    "triangles",
    "lines",
};
static_assert(static_cast<size_t>(Mesh_index_type::Num_Mesh_Index_Type) == array_size(g_meshIndexTypeNames));

std::string g_animationTypeNames[] = { 
    "translation",
    "rotation",
    "scale",
    "morph",
};
static_assert(static_cast<size_t>(Animation_type::Num_Animation_Type) == array_size(g_animationTypeNames));

std::string g_animationInterpolationNames[] = {
    "linear",
    "step",
    "cubic_spline",
};
static_assert(static_cast<size_t>(Animation_interpolation::Num_Animation_Interpolation) == array_size(g_animationInterpolationNames));

std::string g_elementTypeNames[] = {
    "Scalar",
    "Vector2",
    "Vector3",
    "Vector4",
    "Matrix2",
    "Matrix3",
    "Matrix4",
};
static_assert(static_cast<size_t>(Element_type::Num_Element_Type) == array_size(g_elementTypeNames));

std::string g_elementComponentNames[] = {
    "Signed_Byte",
    "Unsigned_Byte",
    "Signed_Short",
    "Unsigned_Short",
    "Signed_Int",
    "Unsigned_Int",
    "Float",
};
static_assert(static_cast<size_t>(Element_component::Num_Element_Component) == array_size(g_elementComponentNames));

std::string g_debugDisplayModeNames[] = {
    "None",
    "Normals",
	"World_Positions",
	"Lighting",
};
static_assert(static_cast<size_t>(Debug_display_mode::Num_Debug_Display_Mode) == array_size(g_debugDisplayModeNames));

const std::string& oe::materialAlphaModeToString(Material_alpha_mode enumValue)
{
    return g_alphaModeNames[static_cast<size_t>(enumValue)];
}

Material_alpha_mode oe::stringToMaterialAlphaMode(const std::string & str)
{
    return stringToEnum<Material_alpha_mode>(str, g_alphaModeNames);
}

const std::string& oe::materialFaceCullModeToString(Material_face_cull_mode enumValue)
{
    return g_faceCullModeNames[static_cast<size_t>(enumValue)];
}

Material_face_cull_mode oe::stringToMaterialFaceCullMode(const std::string& str)
{
    return stringToEnum<Material_face_cull_mode>(str, g_faceCullModeNames);
}

const std::string& oe::materialLightModeToString(Material_light_mode enumValue)
{
    return g_lightModeNames[static_cast<size_t>(enumValue)];
}

Material_light_mode oe::stringToMaterialLightMode(const std::string& str)
{
    return stringToEnum<Material_light_mode>(str, g_lightModeNames);
}

const std::string& oe::renderPassBlendModeToString(Render_pass_blend_mode enumValue)
{
    return g_blendModeNames[static_cast<size_t>(enumValue)];
}

Render_pass_blend_mode oe::stringToRenderPassBlendMode(const std::string& str)
{
    return stringToEnum<Render_pass_blend_mode>(str, g_blendModeNames);
}

const std::string& oe::renderPassDepthModeToString(Render_pass_depth_mode enumValue) 
{
    return g_depthModeNames[static_cast<size_t>(enumValue)];
}

Render_pass_depth_mode oe::stringToRenderPassDepthMode(const std::string& str)
{
    return stringToEnum<Render_pass_depth_mode>(str, g_depthModeNames);
}

const std::string& oe::renderPassStencilModeToString(Render_pass_stencil_mode enumValue)
{
    return g_stencilModeNames[static_cast<size_t>(enumValue)];
}

Render_pass_stencil_mode oe::stringToRenderPassStencilMode(const std::string& str)
{
    return stringToEnum<Render_pass_stencil_mode>(str, g_stencilModeNames);
}

const std::string& oe::vertexAttributeToString(Vertex_attribute enumValue)
{
    return g_vertexAttributeNames[static_cast<size_t>(enumValue)]; 
}

Vertex_attribute oe::stringToVertexAttribute(const std::string& str)
{
    return stringToEnum<Vertex_attribute>(str, g_vertexAttributeNames);
}

const std::string& oe::meshIndexTypeToString(Mesh_index_type enumValue)
{
    return g_meshIndexTypeNames[static_cast<size_t>(enumValue)];
}

Mesh_index_type oe::stringToMeshIndexType(const std::string& str)
{
    return stringToEnum<Mesh_index_type>(str, g_meshIndexTypeNames);
}

const std::string& oe::animationTypeToString(Animation_type enumValue)
{
    return g_animationTypeNames[static_cast<size_t>(enumValue)];
}

Animation_type oe::stringToAnimationType(const std::string& str)
{
    return stringToEnum<Animation_type>(str, g_animationTypeNames);
}

const std::string& oe::animationInterpolationToString(Animation_interpolation enumValue)
{
    return g_animationInterpolationNames[static_cast<size_t>(enumValue)];
}

Animation_interpolation oe::stringToAnimationInterpolation(const std::string& str)
{
    return stringToEnum<Animation_interpolation>(str, g_animationInterpolationNames);
}

const std::string& oe::elementTypeToString(Element_type enumValue)
{
    return g_elementTypeNames[static_cast<size_t>(enumValue)];
}

Element_type oe::stringToElementType(const std::string& str)
{
    return stringToEnum<Element_type>(str, g_elementTypeNames);
}

const std::string& oe::elementComponentToString(Element_component enumValue)
{
    return g_elementComponentNames[static_cast<size_t>(enumValue)];
}

Element_component oe::stringToElementComponent(const std::string& str)
{
    return stringToEnum<Element_component>(str, g_elementComponentNames);
}

const std::string& oe::debugDisplayModeToString(Debug_display_mode enumValue)
{
    return g_debugDisplayModeNames[static_cast<size_t>(enumValue)];
}

Debug_display_mode oe::stringToDebugDisplayMode(const std::string& str)
{
    return stringToEnum<Debug_display_mode>(str, g_debugDisplayModeNames);
}
