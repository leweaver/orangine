#pragma once

namespace oe {
class Scene;
class Binding_helpers {
 public:
  // Gets the scene from the current python context
  static Scene* get_scene();
};
} // namespace oe