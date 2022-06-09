#pragma once

#include "Texture.h"
#include "Renderer_types.h"

namespace oe {
class Render_pass {
 public:
  Render_pass() = default;
  virtual ~Render_pass() = default;
  Render_pass(const Render_pass&) = default;
  Render_pass& operator=(const Render_pass&) = default;
  Render_pass(Render_pass&&) = default;
  Render_pass& operator=(Render_pass&&) = default;

  const std::vector<std::shared_ptr<Texture>>& getCustomRenderTargets() const { return _renderTargets; }
  void setCustomRenderTargets(const std::vector<std::shared_ptr<Texture>>& renderTargets);
  bool popRenderTargetsChanged() {
    const bool wasChanged = _renderTargetsChanged;
    _renderTargetsChanged = false;
    return wasChanged;
  }
  void clearCustomRenderTargets();

  void setDepthStencilConfig(const Depth_stencil_config& config) { _depthStencilConfig = config; }
  Depth_stencil_config getDepthStencilConfig() const { return _depthStencilConfig; }

  virtual void render(const Camera_data& cameraData) = 0;

 protected:
  std::vector<std::shared_ptr<Texture>> _renderTargets = {};
  bool _renderTargetsChanged = false;
  Depth_stencil_config _depthStencilConfig;
};
} // namespace oe