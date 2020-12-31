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
  
  const std::vector<std::shared_ptr<Texture>>& getRenderTargets() const { return _renderTargets; }
  void setRenderTargets(const std::vector<std::shared_ptr<Texture>>& renderTargets);
  bool popRenderTargetsChanged() {
    const bool wasChanged = _renderTargetsChanged;
    _renderTargetsChanged = false;
    return wasChanged;
  }
  void clearRenderTargets();

  void setStencilRef(uint32_t stencilRef) { _stencilRef = stencilRef; }
  uint32_t stencilRef() const { return _stencilRef; }

  void setDepthStencilConfig(const Depth_stencil_config& config) { _depthStencilConfig = config; }
  Depth_stencil_config getDepthStencilConfig() const { return _depthStencilConfig; }

  void setDrawDestination(Render_pass_destination drawDestination) { _drawDestination = drawDestination; }
  Render_pass_destination getDrawDestination() const { return _drawDestination; }

  virtual void render(const Camera_data& cameraData) = 0;

 protected:
  std::vector<std::shared_ptr<Texture>> _renderTargets = {};
  bool _renderTargetsChanged = false;
  Depth_stencil_config _depthStencilConfig;
  Render_pass_destination _drawDestination = Render_pass_destination::Default;

  uint32_t _stencilRef = 0;
};
} // namespace oe