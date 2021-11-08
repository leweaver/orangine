#pragma once

#include <OeCore/Render_pass.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/IEntity_render_manager.h>

#include <memory>

namespace oe {
struct Renderable;
class Scene;
class Skybox_material;

class Render_pass_skybox : public Render_pass {
 public:
  Render_pass_skybox(IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager);

  void render(const Camera_data& cameraData) override;

 private:
  std::shared_ptr<Skybox_material> _material;
  std::unique_ptr<Renderable> _renderable;

  IEntity_render_manager& _entityRenderManager;
  ILighting_manager& _lightingManager;
};
} // namespace oe
