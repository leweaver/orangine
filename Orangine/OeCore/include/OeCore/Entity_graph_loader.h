#pragma once

#include <memory>
#include <string>
#include <vector>

namespace DX {
class DeviceResources;
}

namespace oe {
class IPrimitive_mesh_data_factory;
class Entity;
class IEntity_repository;
class ITexture_manager;
class IMaterial_manager;
class IComponent_factory;
class IScene_graph_manager;

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
          std::string_view filename, IScene_graph_manager& sceneGraphManager, IEntity_repository& entityRepository,
          IComponent_factory& componentFactory, bool calculateBounds) const = 0;
};
}// namespace oe