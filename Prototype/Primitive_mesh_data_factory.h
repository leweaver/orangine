#pragma once
#include "Mesh_data.h"
#include "SimpleMath.h"

namespace oe {
	class Mesh_data;
	struct Mesh_index_buffer_accessor;
	struct Mesh_vertex_buffer_accessor;

	class Primitive_mesh_data_factory
	{
	public:
		std::shared_ptr<Mesh_data> createQuad(const DirectX::SimpleMath::Vector2& size) const;
		std::shared_ptr<Mesh_data> createQuad(const DirectX::SimpleMath::Vector2& size, const DirectX::SimpleMath::Vector3& positionOffset) const;

		/*
		 * Generates flat normals for the given triangles (ie, does not interpolate neighbours)
		 */
		void generateNormals(const Mesh_index_buffer_accessor& indexBufferAccessor,
			const Mesh_vertex_buffer_accessor& positionBufferAccessor,
			Mesh_vertex_buffer_accessor& normalBufferAccessor) const;

		/*
		 * Generates tangents, in MikktSpace
		 */
		void generateTangents(std::shared_ptr<Mesh_data> meshData) const;
	};
}
