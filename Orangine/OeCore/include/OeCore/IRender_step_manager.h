#pragma once

#include "Manager_base.h"
#include "Render_pass.h"
#include "Renderer_types.h"

#include <array>
#include <memory>

namespace oe {
class Camera_component;
class Deferred_light_material;
class Render_light_data;
class Unlit_material;
class Entity_alpha_sorter;
class Entity_cull_sorter;
class Entity_filter;
class Entity;

class IRender_step_manager {
 public:
  // IRender_step_manager
  virtual void createRenderSteps() = 0;
  virtual Camera_data createCameraData(Camera_component& component) const = 0;
  virtual Viewport getScreenViewport() const = 0;
  virtual void render() = 0;
  virtual BoundingFrustumRH createFrustum(const Camera_component& cameraComponent) = 0;
  virtual void setCameraEntity(std::shared_ptr<Entity> cameraEntity) = 0;
};
}// namespace oe
