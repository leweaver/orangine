#pragma once

#include "Component.h"

#include "Mesh_data.h"

/*
 * CPU side mesh vertex, index or animation buffer. Used to create a new mesh.
 */
namespace oe {
class Mesh_bind_context;

class Mesh_data_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  explicit Mesh_data_component(Entity& entity) : Component(entity), _meshData(nullptr) {}

  ~Mesh_data_component() = default;

  // TODO: Will eventually reference an asset ID here, rather than actual mesh data.
  const std::shared_ptr<Mesh_data>& meshData() const { return _meshData; }
  void setMeshData(const std::shared_ptr<Mesh_data>& meshData) { _meshData = meshData; }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  END_COMPONENT_PROPERTIES();

  std::shared_ptr<Mesh_data> _meshData;
};
} // namespace oe
