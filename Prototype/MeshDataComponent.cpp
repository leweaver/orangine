#include "pch.h"
#include "MeshDataComponent.h"

using namespace OE;

std::string VA_POSITION_STR("VA_POSITION");
std::string VA_COLOR_STR("VA_COLOR");
std::string VA_NORMAL_STR("VA_NORMAL");
std::string VA_TANGENT_STR("VA_TANGENT");
std::string VA_TEXCOORD_0_STR("VA_TEXCOORD_0");
std::string VA_INVALID_STR("VA_INVALID");

std::vector<const char *> VA_SEMANTIC_NAMES = {
	"POSITION",
	"COLOR",
	"NORMAL",
	"TANGENT",
	"TEXCOORD",
	""
};

std::vector<int> VA_SEMANTIC_INDICES = {
	0, 0, 0, 0, 0, 0
};

DEFINE_COMPONENT_TYPE(MeshDataComponent);

MeshDataComponent::MeshDataComponent()
	: m_vertexCount(0)
	, m_indexCount(0)
{
}

MeshDataComponent::~MeshDataComponent()
{
}

const std::string &VertexAttributeMeta::str(VertexAttribute attribute)
{
	switch (attribute) {
	case VertexAttribute::VA_POSITION: return VA_POSITION_STR;
	case VertexAttribute::VA_COLOR: return VA_COLOR_STR;
	case VertexAttribute::VA_NORMAL: return VA_NORMAL_STR;
	case VertexAttribute::VA_TANGENT: return VA_TANGENT_STR;
	case VertexAttribute::VA_TEXCOORD_0: return VA_TEXCOORD_0_STR;
	}
	return VA_INVALID_STR;
}

const size_t VertexAttributeMeta::elementSize(VertexAttribute attribute)
{
	switch (attribute)
	{
	case VertexAttribute::VA_TEXCOORD_0:
		return sizeof(float) * 2;

	case VertexAttribute::VA_POSITION:
	case VertexAttribute::VA_COLOR:
	case VertexAttribute::VA_NORMAL:
		return sizeof(float) * 3;

	case VertexAttribute::VA_TANGENT:
		return sizeof(float) * 4;

	default:
		throw std::logic_error("Cannot determine element size for VertexAttribute: " + str(attribute));
	}
}

const char *VertexAttributeMeta::semanticName(VertexAttribute attribute)
{
	return VA_SEMANTIC_NAMES.at(static_cast<UINT>(attribute));
}

UINT VertexAttributeMeta::semanticIndex(VertexAttribute attribute)
{
	return VA_SEMANTIC_INDICES.at(static_cast<UINT>(attribute));
}

MeshBuffer::MeshBuffer(size_t size)
	: m_data(nullptr)
	, m_dataSize(size)
{
	m_data = new std::uint8_t[size];
}

MeshBuffer::~MeshBuffer()
{
	if (m_data != nullptr)
		delete[] m_data;
	m_data = nullptr;
	m_dataSize = 0;
}

MeshBufferAccessor::MeshBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, unsigned count, unsigned stride, unsigned offset)
	: m_buffer(buffer)
	, m_count(count)
	, m_stride(stride)
	, m_offset(offset)
{
}

MeshBufferAccessor::MeshBufferAccessor()
	: m_buffer(nullptr)
	, m_count(0)
	, m_stride(0)
	, m_offset(0)
{
}

MeshBufferAccessor::~MeshBufferAccessor()
{
	m_buffer = nullptr;
}

MeshVertexBufferAccessor::MeshVertexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, VertexAttribute attribute, unsigned count, unsigned stride, unsigned offset)
	: MeshBufferAccessor(buffer, count, stride, offset)
	, m_attribute(attribute)
{
}

MeshVertexBufferAccessor::~MeshVertexBufferAccessor()
{
}

MeshIndexBufferAccessor::MeshIndexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, DXGI_FORMAT format, unsigned count, unsigned stride, unsigned offset)
	: MeshBufferAccessor(buffer, count, stride, offset)
	, m_format(format)
{
}

MeshIndexBufferAccessor::~MeshIndexBufferAccessor()
{
}

