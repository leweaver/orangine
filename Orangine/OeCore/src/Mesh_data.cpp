#include "pch.h"
#include "OeCore/Mesh_data.h"
#include <utility>
#include "OeCore/EngineUtils.h"

using namespace oe;
using namespace std::string_literals;

const std::string g_position_str("VA_POSITION");
const std::string g_color_str("VA_COLOR");
const std::string g_normal_str("VA_NORMAL");
const std::string g_tangent_str("VA_TANGENT");
const std::string g_bi_tangent_str("VA_BITANGENT");
const std::string g_texcoord_str("VA_TEXCOORD");
const std::string g_joints_str("VA_TEXCOORD");
const std::string g_weights_str("VA_TEXCOORD");
const std::string g_invalid_str("VA_INVALID");

std::string g_va_semantic_names[] = {
	"POSITION",
	"COLOR",
	"NORMAL",
	"TANGENT",
	"BITANGENT",
	"TEXCOORD",
    "BONEINDICES",
    "WEIGHTS"
};

static_assert(array_size(g_va_semantic_names) == static_cast<uint32_t>(Vertex_attribute::Num_Vertex_Attribute));

uint32_t Mesh_data::getVertexCount() const
{
	if (vertexBufferAccessors.empty())
		return 0;
	return vertexBufferAccessors.begin()->second->count;
}

std::string Vertex_attribute_meta::vsInputName(Vertex_attribute_semantic attribute)
{
    const auto& str = [](Vertex_attribute_semantic& attribute) {
        switch (attribute.attribute) {
        case Vertex_attribute::Position: return g_position_str;
        case Vertex_attribute::Color: return g_color_str;
        case Vertex_attribute::Normal: return g_normal_str;
        case Vertex_attribute::Tangent: return g_tangent_str;
        case Vertex_attribute::Bi_Tangent: return g_bi_tangent_str;
        case Vertex_attribute::Tex_Coord: return g_texcoord_str;

        case Vertex_attribute::Num_Vertex_Attribute:
        default:
            return g_invalid_str;
        }
    }(attribute);

    if (attribute.semanticIndex != 0) {
        return str + "_"s + std::to_string(attribute.semanticIndex);
    }
    return str;
}

size_t Vertex_attribute_meta::numComponents(Vertex_attribute attribute)
{
	switch (attribute)
	{
	case Vertex_attribute::Tex_Coord:
		return 2;

	case Vertex_attribute::Position:
	case Vertex_attribute::Color:
	case Vertex_attribute::Normal:
    case Vertex_attribute::Bi_Tangent:
		return 3;

	case Vertex_attribute::Tangent:
		return 4;

	default:
		OE_THROW(std::logic_error("Cannot determine element size for VertexAttribute: "s.append(vertexAttributeToString(attribute))));
	}
}

std::string_view Vertex_attribute_meta::semanticName(Vertex_attribute attribute)
{
    const auto attrIndex = static_cast<uint32_t>(attribute);
    assert(attrIndex < array_size(g_va_semantic_names));

	return g_va_semantic_names[attrIndex];
}

Mesh_buffer::Mesh_buffer(size_t size)
	: data(nullptr)
	, dataSize(size)
{
	data = new std::uint8_t[size];
}

Mesh_buffer::~Mesh_buffer()
{
	delete[] data;
	data = nullptr;
	dataSize = 0;
}

Mesh_buffer_accessor::Mesh_buffer_accessor(std::shared_ptr<Mesh_buffer> buffer, uint32_t count, uint32_t stride, uint32_t offset)
	: buffer(std::move(buffer))
	, count(count)
	, stride(stride)
	, offset(offset)
{
}

Mesh_buffer_accessor::Mesh_buffer_accessor()
	: buffer(nullptr)
	, count(0)
	, stride(0)
	, offset(0)
{
}

Mesh_buffer_accessor::~Mesh_buffer_accessor()
{
	buffer = nullptr;
}

Mesh_vertex_buffer_accessor::Mesh_vertex_buffer_accessor(const std::shared_ptr<Mesh_buffer> &buffer, Vertex_attribute_element attributeElement, uint32_t count, uint32_t stride, uint32_t offset)
	: Mesh_buffer_accessor(buffer, count, stride, offset)
	, attributeElement(attributeElement)
{
}

Mesh_index_buffer_accessor::Mesh_index_buffer_accessor(const std::shared_ptr<Mesh_buffer> &buffer, Element_component component, uint32_t count, uint32_t stride, uint32_t offset)
	: Mesh_buffer_accessor(buffer, count, stride, offset)
	, component(component)
{
}
