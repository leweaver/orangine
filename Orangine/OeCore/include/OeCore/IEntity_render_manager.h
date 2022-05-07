#pragma once

#include "Collision.h"
#include "Deferred_light_material.h"
#include "Manager_base.h"
#include "IPrimitive_mesh_data_factory.h"

#include "Environment_volume.h"
#include "Light_provider.h"
#include <memory>

namespace oe {
class Unlit_material;
class Camera_component;
class Renderable_component;
class Scene;
class Material;
class Entity_filter;
class Entity_alpha_sorter;
class Entity_cull_sorter;
class Shadow_map_texture_pool;
struct Renderable;

class IEntity_render_manager {
 public:
  virtual void renderRenderable(
          Renderable& renderable, const SSE::Matrix4& worldMatrix, float radius, const Camera_data& cameraData,
          const Light_provider::Callback_type& lightDataProvider, const Depth_stencil_config& depthStencilConfig, bool wireFrame) = 0;

  virtual void renderEntity(
          Renderable_component& renderable, const Camera_data& cameraData,
          const Light_provider::Callback_type& lightDataProvider, const Depth_stencil_config& depthStencilConfig) = 0;

  virtual Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) = 0;
  virtual void clearRenderStats() = 0;
};
}// namespace oe