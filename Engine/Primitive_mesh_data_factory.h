#pragma once
#include "Mesh_data.h"
#include <SimpleMath.h>

namespace oe {
	struct BoundingFrustumRH;
	class Mesh_data;
	struct Mesh_index_buffer_accessor;
	struct Mesh_vertex_buffer_accessor;

	class Primitive_mesh_data_factory
	{
	public:
		// Creates an indexed mesh with Position
		std::shared_ptr<Mesh_data> createQuad(const DirectX::SimpleMath::Vector2& size) const;
		std::shared_ptr<Mesh_data> createQuad(const DirectX::SimpleMath::Vector2& size, const DirectX::SimpleMath::Vector3& positionOffset) const;

		// Creates an indexed mesh with Position, Normal, Tangent, Textcoord_0
		std::shared_ptr<Mesh_data> createBox(const DirectX::SimpleMath::Vector3& size) const;
		std::shared_ptr<Mesh_data> createTeapot(size_t tessellation = 8) const;
		std::shared_ptr<Mesh_data> createSphere(float radius = 0.5f, size_t tessellation = 16, bool invertNormals = false) const;

		std::shared_ptr<Mesh_data> createFrustumLines(const BoundingFrustumRH& frustum);

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
