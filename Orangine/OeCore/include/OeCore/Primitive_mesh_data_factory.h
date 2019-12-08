#pragma once
#include "Mesh_data.h"
#include <vectormath/vectormath.hpp>

namespace oe {
	struct BoundingFrustumRH;
	class Mesh_data;
	struct Mesh_index_buffer_accessor;
	struct Mesh_vertex_buffer_accessor;

	class Primitive_mesh_data_factory
	{
	public:
		// Creates an indexed mesh with Position
		static std::shared_ptr<Mesh_data> createQuad(float width, float height);
		static std::shared_ptr<Mesh_data> createQuad(float width, float height, const SSE::Vector3& positionOffset);

		// Creates an indexed mesh with Position, Normal, Tangent, Textcoord_0
		static std::shared_ptr<Mesh_data> createBox(const SSE::Vector3& size);
        static std::shared_ptr<Mesh_data> createCone(float diameter, float height, size_t tessellation = 32);
		static std::shared_ptr<Mesh_data> createTeapot(size_t tessellation = 8);
		static std::shared_ptr<Mesh_data> createSphere(float radius = 0.5f, size_t tessellation = 16, bool invertNormals = false);

		static std::shared_ptr<Mesh_data> createFrustumLines(const BoundingFrustumRH& frustum);
        static std::shared_ptr<Mesh_data> createAxisWidgetLines();

		/*
		 * Generates flat normals for the given triangles (ie, does not interpolate neighbours)
		 */
		static void generateNormals(const Mesh_index_buffer_accessor& indexBufferAccessor,
			const Mesh_vertex_buffer_accessor& positionBufferAccessor,
			Mesh_vertex_buffer_accessor& normalBufferAccessor);

		/*
		 * Generates tangents, in MikktSpace
		 */
		static void generateTangents(std::shared_ptr<Mesh_data> meshData);
	};
}
