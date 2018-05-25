#pragma once
#include <cstdint>

namespace oe
{
	struct Mesh_index_buffer_accessor;
	struct Mesh_vertex_buffer_accessor;

	enum class Vertex_attribute : std::int8_t
	{
		Position = 0,
		Color,
		Normal,
		Tangent,
		Bi_Tangent,
		Texcoord_0,

		Invalid
	};

	enum class Mesh_index_type : std::int8_t
	{
		Triangles
	};

	class Mesh_data
	{
	public:

		std::map<Vertex_attribute, std::unique_ptr<Mesh_vertex_buffer_accessor>> vertexBufferAccessors;
		uint32_t getVertexCount() const;

		std::unique_ptr<Mesh_index_buffer_accessor> indexBufferAccessor;
		Mesh_index_type m_meshIndexType = Mesh_index_type::Triangles;
	};

	struct Vertex_attribute_meta {
		static std::string_view str(Vertex_attribute attribute);
		static size_t elementSize(Vertex_attribute attribute);
		static std::string_view semanticName(Vertex_attribute attribute);
		static uint32_t semanticIndex(Vertex_attribute attribute);
	};

	struct Mesh_buffer
	{
		Mesh_buffer(size_t size);
		~Mesh_buffer();

		uint8_t* data;
		size_t dataSize;
	};

	struct Mesh_buffer_accessor
	{
		Mesh_buffer_accessor(std::shared_ptr<Mesh_buffer> buffer, uint32_t count, uint32_t stride, uint32_t offset);
		Mesh_buffer_accessor();
		virtual ~Mesh_buffer_accessor();

		std::shared_ptr<Mesh_buffer> buffer;
		uint32_t count;
		uint32_t stride;
		uint32_t offset;
	};

	struct Mesh_vertex_buffer_accessor : Mesh_buffer_accessor
	{
		Mesh_vertex_buffer_accessor(const std::shared_ptr<Mesh_buffer>& buffer, Vertex_attribute attribute, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~Mesh_vertex_buffer_accessor() = default;

		Vertex_attribute attribute;
	};


	struct Mesh_index_buffer_accessor : Mesh_buffer_accessor
	{
		Mesh_index_buffer_accessor(const std::shared_ptr<Mesh_buffer>& buffer, DXGI_FORMAT format, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~Mesh_index_buffer_accessor() = default;

		DXGI_FORMAT format;
	};
}
