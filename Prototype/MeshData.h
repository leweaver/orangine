#pragma once
#include <cstdint>

namespace OE
{
	struct MeshIndexBufferAccessor;
	struct MeshVertexBufferAccessor;

	enum class VertexAttribute : std::int8_t
	{
		VA_POSITION = 0,
		VA_COLOR,
		VA_NORMAL,
		VA_TANGENT,
		VA_TEXCOORD_0,

		VA_INVALID
	};

	class MeshData
	{
	public:

		std::map<VertexAttribute, std::unique_ptr<MeshVertexBufferAccessor>> m_vertexBufferAccessors;
		uint32_t getVertexCount() const;

		std::unique_ptr<MeshIndexBufferAccessor> m_indexBufferAccessor;
	};

	struct VertexAttributeMeta {
		static const std::string &str(VertexAttribute attribute);
		static size_t elementSize(VertexAttribute attribute);
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
		MeshBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, uint32_t count, uint32_t stride, uint32_t offset);
		MeshBufferAccessor();
		virtual ~MeshBufferAccessor();

		std::shared_ptr<MeshBuffer> m_buffer;
		unsigned int m_count;
		unsigned int m_stride;
		unsigned int m_offset;
	};

	struct MeshVertexBufferAccessor : MeshBufferAccessor
	{
		MeshVertexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, VertexAttribute attribute, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~MeshVertexBufferAccessor();

		VertexAttribute m_attribute;
	};


	struct MeshIndexBufferAccessor : MeshBufferAccessor
	{
		MeshIndexBufferAccessor(const std::shared_ptr<MeshBuffer> &buffer, DXGI_FORMAT format, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~MeshIndexBufferAccessor();

		DXGI_FORMAT m_format;
	};
}
