#pragma once

#include "D3D_device_repository.h"
#include "OeCore/Render_step_manager.h"

namespace oe {

class D3D_render_step_manager : public Render_step_manager {
 public:
  struct D3D_render_pass_data {
    std::vector<ID3D11RenderTargetView*> _renderTargetViews = {};
    Microsoft::WRL::ComPtr<ID3D11BlendState> _blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilState;
  };
  struct D3D_render_step_data {
    std::vector<D3D_render_pass_data> renderPassData;
  };

  D3D_render_step_manager(Scene& scene, std::shared_ptr<D3D_device_repository> device_repository);

  // Base class overrides
  void shutdown() override;
  Viewport getScreenViewport() const override;

  // IRender_step_manager implementation
  void clearRenderTargetView(const Color& color) override;
  void clearDepthStencil(float depth, uint8_t stencil) override;
  std::unique_ptr<Render_pass> createShadowMapRenderPass() override;
  void beginRenderNamedEvent(const wchar_t* name) override;
  void endRenderNamedEvent() override;
  void createRenderStepResources() override;
  void destroyRenderStepResources() override;
  void renderSteps(const Camera_data& cameraData) override;

 private:
  DX::DeviceResources& getDeviceResources() const;

  void createRenderStepResources(Render_step& step, D3D_render_step_data& renderStepData);

  void destroyRenderStepResources(Render_step& step, D3D_render_step_data& renderStepData);

  void renderStep(
      Render_step& step,
      D3D_render_step_data& renderStepData,
      const Camera_data& cameraData);

  std::vector<D3D_render_step_data> _renderStepData;
  std::vector<std::wstring> _passNames;
  std::shared_ptr<D3D_device_repository> _deviceRepository;
};
} // namespace oe