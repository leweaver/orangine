#pragma once

#include <memory>
#include <vector>
#include <string>

namespace DX {
class DeviceResources;
}

namespace oe {
class ITexture_manager;
class Primitive_mesh_data_factory;
class Entity;
class IEntity_repository;
class IMaterial_repository;

class Entity_graph_loader {
 public:
  virtual ~Entity_graph_loader() = default;

  virtual void getSupportedFileExtensions(std::vector<std::string>& extensions) const = 0;

  /**
   * Returns a vector of root entities, that may have children. These entities will have been
   * initialized such that they point to a scene, but they will not have been added to that scene
   * yet.
   */
  virtual std::vector<std::shared_ptr<Entity>> loadFile(
      std::wstring_view filename,
      IEntity_repository& entityRepository,
      IMaterial_repository& materialRepository,
      ITexture_manager& textureManager,
      bool calculateBounds) const = 0;
};
} // namespace oe