#pragma once

#include "Renderer_enum.h"
#include "Mesh_vertex_layout.h"
#include "EngineUtils.h"

#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <stdexcept>

namespace oe
{
	struct Mesh_index_buffer_accessor;
	struct Mesh_vertex_buffer_accessor;

	class Mesh_data
	{
	public:
        explicit Mesh_data(Mesh_vertex_layout meshVertexLayout) : vertexLayout(meshVertexLayout) {}

        uint32_t getVertexCount() const;

        Mesh_vertex_layout vertexLayout;

		std::map<Vertex_attribute_semantic, std::unique_ptr<Mesh_vertex_buffer_accessor>> vertexBufferAccessors;

        /**
         * Outer array is morphTargetIndex,
         * inner array is attributeLayoutIndex.
         */
        std::vector<std::vector<std::unique_ptr<Mesh_vertex_buffer_accessor>>> attributeMorphBufferAccessors;
        
		std::unique_ptr<Mesh_index_buffer_accessor> indexBufferAccessor;
		Mesh_index_type m_meshIndexType = Mesh_index_type::Triangles;
	};

	struct Vertex_attribute_meta {
		static std::string vsInputName(Vertex_attribute_semantic attribute);
		static size_t numComponents(Vertex_attribute attribute);
		static std::string_view semanticName(Vertex_attribute attribute);
	};

	struct Mesh_buffer
	{
		explicit Mesh_buffer(size_t size);
		~Mesh_buffer();

        const uint8_t* getIndexed(size_t index, size_t stride, size_t offset) const
        {
            const auto startPos = offset + index * stride;
            const auto endPos = startPos + stride;
            if (startPos > dataSize || endPos > dataSize) {
              OE_THROW(std::runtime_error(
                  "Illegal buffer access! index=" + std::to_string(index) +
                  ", stride=" + std::to_string(stride) + ", offset=" + std::to_string(offset) +
                  ", dataSize=" + std::to_string(dataSize)));
            }
            return data + startPos;
        }

		uint8_t* data;
		size_t dataSize;
	};

	struct Mesh_buffer_accessor
	{
		Mesh_buffer_accessor(std::shared_ptr<Mesh_buffer> buffer, uint32_t count, uint32_t stride, uint32_t offset);
		Mesh_buffer_accessor();
		virtual ~Mesh_buffer_accessor();

        const uint8_t* getIndexed(size_t index) const
        {
            return buffer->getIndexed(index, stride, offset);
        }

		std::shared_ptr<Mesh_buffer> buffer;
		uint32_t count;
		uint32_t stride;
		uint32_t offset;
	};

	struct Mesh_vertex_buffer_accessor : Mesh_buffer_accessor
	{
		Mesh_vertex_buffer_accessor(const std::shared_ptr<Mesh_buffer>& buffer, Vertex_attribute_element attribute, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~Mesh_vertex_buffer_accessor() = default;

        const Vertex_attribute_element attributeElement;
	};

	struct Mesh_index_buffer_accessor : Mesh_buffer_accessor
	{
		Mesh_index_buffer_accessor(const std::shared_ptr<Mesh_buffer>& buffer, Element_component component, uint32_t count, uint32_t stride, uint32_t offset);
		virtual ~Mesh_index_buffer_accessor() = default;

        const Element_component component;
	};
}
