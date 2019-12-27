#pragma once

#include "Render_target_texture.h"

namespace oe {
class Render_target_view_texture;

class Render_pass {
 public:
  struct Camera_data {
    SSE::Matrix4 viewMatrix;
    SSE::Matrix4 projectionMatrix;
    float fov;
    float aspectRatio;
    bool enablePixelShader = true;

    static const Camera_data IDENTITY;
  };

  Render_pass() = default;
  virtual ~Render_pass() = default;
  Render_pass(const Render_pass&) = default;
  Render_pass& operator=(const Render_pass&) = default;
  Render_pass(Render_pass&&) = default;
  Render_pass& operator=(Render_pass&&) = default;

  void setBlendState(ID3D11BlendState* blendState);
  ID3D11BlendState* blendState() const { return _blendState.Get(); }

  void setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>&& renderTargets);
  void clearRenderTargets();
  virtual void render(const Camera_data& cameraData) = 0;

  std::tuple<ID3D11RenderTargetView* const*, size_t> renderTargetViewArray() const;

  void setDepthStencilState(ID3D11DepthStencilState* depthStencilState);
  ID3D11DepthStencilState* depthStencilState() const { return _depthStencilState.Get(); }

  void setStencilRef(uint32_t stencilRef) { _stencilRef = stencilRef; }
  uint32_t stencilRef() const { return _stencilRef; }

  virtual void createDeviceDependentResources() {}
  virtual void destroyDeviceDependentResources() {}

 protected:
  std::vector<std::shared_ptr<Render_target_view_texture>> _renderTargets = {};
  std::vector<ID3D11RenderTargetView*> _renderTargetViews = {};
  Microsoft::WRL::ComPtr<ID3D11BlendState> _blendState;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilState;
  uint32_t _stencilRef = 0;
};
} // namespace oe