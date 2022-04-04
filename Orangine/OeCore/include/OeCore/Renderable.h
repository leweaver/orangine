#pragma once

#include <memory>

#include "Material_context.h"

namespace oe {
class Mesh_data;
class Material;
struct Mesh_gpu_data;
struct Renderer_animation_data;

/**
 * A lower level construct for rendering mesh data in immediate mode.
 * YOU are responsible for cleaning up the D3D data attached to instances
 * of this object upon device reset.
 */
struct Renderable {
  std::shared_ptr<Mesh_data> meshData;
  std::shared_ptr<Material> material;
  std::shared_ptr<Renderer_animation_data> rendererAnimationData;

  // The following two properties will be deleted by managers on a device reset.
  std::weak_ptr<Mesh_gpu_data> rendererData;
  std::weak_ptr<Material_context> materialContext;
};
} // namespace oe
