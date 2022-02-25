#pragma once

#include <OeCore/IPrimitive_mesh_data_factory.h>
#include <OeCore/Mesh_data.h>
#include <OeCore/Manager_base.h>

#include <vectormath.hpp>

namespace oe {
struct BoundingFrustumRH;
class Mesh_data;
struct Mesh_index_buffer_accessor;
struct Mesh_vertex_buffer_accessor;

class Primitive_mesh_data_factory : public IPrimitive_mesh_data_factory, public Manager_base {
 public:

  // Manager_base implementations
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override
  {
    return _name;
  }

  static void initStatics();
  static void destroyStatics() {}

  // Creates an indexed mesh with Position
  std::shared_ptr<Mesh_data> createQuad(float width, float height) override;
  std::shared_ptr<Mesh_data>
  createQuad(float width, float height, const SSE::Vector3& positionOffset) override;

  // Creates an indexed mesh with Position, Normal, Tangent, Textcoord_0
  std::shared_ptr<Mesh_data> createBox(const SSE::Vector3& size) override;
  std::shared_ptr<Mesh_data>
  createCone(float diameter, float height, size_t tessellation = 32) override;
  std::shared_ptr<Mesh_data> createTeapot(size_t tessellation = 8) override;
  std::shared_ptr<Mesh_data>
  createSphere(float radius = 0.5f, size_t tessellation = 16, bool invertNormals = false) override;

  std::shared_ptr<Mesh_data> createFrustumLines(const BoundingFrustumRH& frustum) override;
  std::shared_ptr<Mesh_data> createAxisWidgetLines() override;

  /*
   * Generates flat normals for the given triangles (ie, does not interpolate neighbours)
   */
  void generateNormals(
          const Mesh_index_buffer_accessor& indexBufferAccessor,
          const Mesh_vertex_buffer_accessor& positionBufferAccessor,
          Mesh_vertex_buffer_accessor& normalBufferAccessor) override;

  /*
   * Generates tangents, in MikktSpace
   */
  void generateTangents(std::shared_ptr<Mesh_data> meshData) override;

 private:
  static std::string _name;
};
} // namespace oe
