#pragma once

#include <memory>

#include "Material_context.h"

// TODO: Remove this
#include "D3D11/D3D_renderer_data.h"

namespace oe {
class Mesh_data;
class Material;
struct Renderer_data;
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

  // The following two properties must be manually reset on a device reset.
  std::unique_ptr<Renderer_data> rendererData;
  std::weak_ptr<Material_context> materialContext;
};
} // namespace oe
