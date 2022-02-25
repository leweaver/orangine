#pragma once
#include "Mesh_data.h"
#include <vectormath.hpp>

namespace oe {
struct BoundingFrustumRH;
class Mesh_data;
struct Mesh_index_buffer_accessor;
struct Mesh_vertex_buffer_accessor;

class IPrimitive_mesh_data_factory {
 public:
  virtual ~IPrimitive_mesh_data_factory() = default;

  virtual std::shared_ptr<Mesh_data> createQuad(float width, float height) = 0;
  virtual std::shared_ptr<Mesh_data> createQuad(float width, float height, const SSE::Vector3& positionOffset) = 0;

  // Creates an indexed mesh with Position, Normal, Tangent, Textcoord_0
  virtual std::shared_ptr<Mesh_data> createBox(const SSE::Vector3& size) = 0;
  virtual std::shared_ptr<Mesh_data> createCone(float diameter, float height, size_t tessellation = 32) = 0;
  virtual std::shared_ptr<Mesh_data> createTeapot(size_t tessellation = 8) = 0;
  virtual std::shared_ptr<Mesh_data>
  createSphere(float radius = 0.5f, size_t tessellation = 16, bool invertNormals = false) = 0;

  virtual std::shared_ptr<Mesh_data> createFrustumLines(const BoundingFrustumRH& frustum) = 0;
  virtual std::shared_ptr<Mesh_data> createAxisWidgetLines() = 0;

  /*
   * Generates flat normals for the given triangles (ie, does not interpolate neighbours)
   */
  virtual void generateNormals(
          const Mesh_index_buffer_accessor& indexBufferAccessor,
          const Mesh_vertex_buffer_accessor& positionBufferAccessor,
          Mesh_vertex_buffer_accessor& normalBufferAccessor) = 0;

  /*
   * Generates tangents, in MikktSpace
   */
  virtual void generateTangents(std::shared_ptr<Mesh_data> meshData) = 0;
};
} // namespace oe
