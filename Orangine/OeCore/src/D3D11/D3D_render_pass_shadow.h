#pragma once

#include "OeCore/Render_pass.h"

#include <memory>
#include <vector>

#include "D3D_directx11.h"


namespace oe {

class Entity_filter;
class Scene;
class D3D12_device_resources;

class Render_pass_shadow : public Render_pass {
 public:
  Render_pass_shadow(
      Scene& scene,
      std::shared_ptr<D3D12_device_resources> device_repository,
      size_t maxRenderTargetViews);

  void render(const Camera_data& cameraData) override;

 private:
  Scene& _scene;

  std::shared_ptr<Entity_filter> _renderableEntities;
  std::shared_ptr<Entity_filter> _lightEntities;

  std::vector<ID3D11RenderTargetView*> _renderTargetViews;
  std::shared_ptr<oe::D3D12_device_resources> _deviceRepository;
};
} // namespace oe
