#include "pch.h"
#include "Mesh_data.h"
#include <utility>

using namespace oe;
using namespace std::string_literals;

const std::string g_position_str("VA_POSITION");
const std::string g_color_str("VA_COLOR");
const std::string g_normal_str("VA_NORMAL");
const std::string g_tangent_str("VA_TANGENT");
const std::string g_bi_tangent_str("VA_BITANGENT");
const std::string g_texcoord_0_str("VA_TEXCOORD_0");
const std::string g_invalid_str("VA_INVALID");

std::vector<std::string> g_va_semantic_names = {
	"POSITION",
	"COLOR",
	"NORMAL",
	"TANGENT",
	"BITANGENT",
	"TEXCOORD",
	""
};

std::vector<uint32_t> g_va_semantic_indices = {
	0, 0, 0, 0, 0, 0, 0
};

uint32_t Mesh_data::getVertexCount() const
{
	if (vertexBufferAccessors.empty())
		return 0;
	return vertexBufferAccessors.begin()->second->count;
}

std::string_view Vertex_attribute_meta::str(Vertex_attribute attribute)
{
	switch (attribute) {
		case Vertex_attribute::Position: return g_position_str;
		case Vertex_attribute::Color: return g_color_str;
		case Vertex_attribute::Normal: return g_normal_str;
		case Vertex_attribute::Tangent: return g_tangent_str;
		case Vertex_attribute::Bi_Tangent: return g_bi_tangent_str;
		case Vertex_attribute::Texcoord_0: return g_texcoord_0_str;

		case Vertex_attribute::Invalid:
		default:
			return g_invalid_str;
	}
}

size_t Vertex_attribute_meta::elementSize(Vertex_attribute attribute)
{
	switch (attribute)
	{
	case Vertex_attribute::Texcoord_0:
		return sizeof(float) * 2;

	case Vertex_attribute::Position:
	case Vertex_attribute::Color:
	case Vertex_attribute::Normal:
		return sizeof(float) * 3;

	case Vertex_attribute::Tangent:
	case Vertex_attribute::Bi_Tangent:
		return sizeof(float) * 4;

	default:
		throw std::logic_error("Cannot determine element size for VertexAttribute: "s.append(str(attribute)));
	}
}

std::string_view Vertex_attribute_meta::semanticName(Vertex_attribute attribute)
{
	return g_va_semantic_names.at(static_cast<uint32_t>(attribute));
}

uint32_t Vertex_attribute_meta::semanticIndex(Vertex_attribute attribute)
{
	return g_va_semantic_indices.at(static_cast<uint32_t>(attribute));
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

Mesh_vertex_buffer_accessor::Mesh_vertex_buffer_accessor(const std::shared_ptr<Mesh_buffer> &buffer, Vertex_attribute attribute, uint32_t count, uint32_t stride, uint32_t offset)
	: Mesh_buffer_accessor(buffer, count, stride, offset)
	, attribute(attribute)
{
}

Mesh_index_buffer_accessor::Mesh_index_buffer_accessor(const std::shared_ptr<Mesh_buffer> &buffer, DXGI_FORMAT format, uint32_t count, uint32_t stride, uint32_t offset)
	: Mesh_buffer_accessor(buffer, count, stride, offset)
	, format(format)
{
}
