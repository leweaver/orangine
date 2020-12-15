#pragma once

#include <OeCore/Render_pass.h>

#include <memory>

namespace oe {
struct Renderable;
class Scene;
class Skybox_material;

class Render_pass_skybox : public Render_pass {
 public:
  explicit Render_pass_skybox(Scene& scene);

  void render(const Camera_data& cameraData) override;

 private:
  Scene& _scene;

  std::shared_ptr<Skybox_material> _material;
  std::unique_ptr<Renderable> _renderable;
};
} // namespace oe
