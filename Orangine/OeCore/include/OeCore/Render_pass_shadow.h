#pragma once

#include "Render_light_data.h"
#include "Render_pass.h"

#include <memory>
#include <vector>

namespace oe {

class Entity_filter;
class Scene;

namespace internal {
class Device_repository;
}

class Render_pass_shadow : public Render_pass {
 public:
  Render_pass_shadow(
      Scene& scene,
      std::shared_ptr<internal::Device_repository> device_repository,
      size_t maxRenderTargetViews);

  void render(const Camera_data& cameraData) override;

 private:
  Scene& _scene;

  std::shared_ptr<Entity_filter> _renderableEntities;
  std::shared_ptr<Entity_filter> _lightEntities;

  std::vector<ID3D11RenderTargetView*> _renderTargetViews;
  std::shared_ptr<oe::internal::Device_repository> _deviceRepository;
};
} // namespace oe
