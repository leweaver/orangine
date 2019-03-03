#pragma once

namespace oe {

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
    
    enum class Vertex_attribute : std::int8_t
    {
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

        bool operator<(const Vertex_attribute_semantic& other) const
        {
            if (attribute == other.attribute)
                return semanticIndex < other.semanticIndex;
            return attribute < other.attribute;
        }
        bool operator==(const Vertex_attribute_semantic& other) const
        {
            return attribute == other.attribute && semanticIndex == other.semanticIndex;
        }
        bool operator!=(const Vertex_attribute_semantic& other) const
        {
            return !(*this == other);
        }
    };

    enum class Mesh_index_type : std::int8_t
    {
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

    struct Vertex_attribute_element {
        Vertex_attribute_semantic semantic;
        Element_type type;
        Element_component component;
    };
}