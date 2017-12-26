#pragma once

#include "Component.h"

#include <cstdint>
#include <vector>

/*
 * CPU side mesh vertex, index or animation buffer. Used to create a new mesh. 
 */
namespace OE 
{
	struct MeshIndexBufferAccessor;
	struct MeshVertexBufferAccessor;

	class MeshDataComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

	public:
		MeshDataComponent();
		~MeshDataComponent();

		std::vector<std::unique_ptr<MeshVertexBufferAccessor>> m_vertexBufferAccessors;
		unsigned int m_vertexCount;

		std::unique_ptr<MeshIndexBufferAccessor> m_indexBufferAccessor;
		unsigned int m_indexCount;
	};

	enum class VertexAttribute : std::int8_t
	{
		VA_POSITION = 0,
		VA_COLOR,
		VA_NORMAL,
		VA_TANGENT,
		VA_TEXCOORD_0,

		VA_INVALID
	};

	struct VertexAttributeMeta {
		static const std::string &str(VertexAttribute attribute);
		static const size_t elementSize(VertexAttribute attribute);
		static const char *VertexAttributeMeta::semanticName(VertexAttribute attribute);
		static UINT VertexAttributeMeta::semanticIndex(VertexAttribute attribute);
	};
	
	struct MeshBuffer
	{
		MeshBuffer(size_t size);
		~MeshBuffer();

		std::uint8_t *m_data;
		std::size_t m_dataSize;
	};

	struct MeshBufferAccessor
	{
		MeshBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, unsigned count, unsigned stride, unsigned offset);
		MeshBufferAccessor();
		virtual ~MeshBufferAccessor();

		std::shared_ptr<MeshBuffer> m_buffer;
		unsigned int m_count;
		unsigned int m_stride;
		unsigned int m_offset;
	};

	struct MeshVertexBufferAccessor : public MeshBufferAccessor
	{
		MeshVertexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, VertexAttribute attribute, unsigned count, unsigned stride, unsigned offset);
		virtual ~MeshVertexBufferAccessor();

		VertexAttribute m_attribute;
	};


	struct MeshIndexBufferAccessor : public MeshBufferAccessor
	{
		MeshIndexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, DXGI_FORMAT format, unsigned count, unsigned stride, unsigned offset);
		virtual ~MeshIndexBufferAccessor();

		DXGI_FORMAT m_format;
	};
}

