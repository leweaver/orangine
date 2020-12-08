#include "pch.h"

#include "D3D_device_repository.h"
#include "D3D_render_pass_shadow.h"
#include "D3D_render_step_manager.h"

#include "OeCore/Perf_timer.h"


#include <OeCore/Color.h>
#include <OeCore/Render_pass_generic.h>
#include <OeCore/Render_pass_skybox.h>

#include <CommonStates.h>

using namespace DirectX;

using oe::D3D_render_step_manager;
using oe::Render_pass_blend_mode;
using oe::Render_pass_depth_mode;
using oe::Render_pass_stencil_mode;

template <>
oe::IRender_step_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<D3D_device_repository>& device_repository) {
  return new D3D_render_step_manager(scene, device_repository);
}

D3D_render_step_manager::D3D_render_step_manager(
    Scene& scene,
    std::shared_ptr<D3D_device_repository> device_repository)
    : Render_step_manager(scene), _deviceRepository(device_repository) {}

void D3D_render_step_manager::shutdown() {

  auto context = getDeviceResources().GetD3DDeviceContext();
  if (context) {
    std::array<ID3D11RenderTargetView*, maxRenderTargetViews()> renderTargetViews = {
        nullptr, nullptr, nullptr};
    context->OMSetRenderTargets(
        static_cast<UINT>(renderTargetViews.size()), renderTargetViews.data(), nullptr);
  }
}

inline DX::DeviceResources& D3D_render_step_manager::getDeviceResources() const {
  return _deviceRepository->deviceResources();
}

void D3D_render_step_manager::clearRenderTargetView(const Color& color) {
  auto& deviceResources = getDeviceResources();
  auto* const view = deviceResources.GetRenderTargetView();
  auto colorFloats = static_cast<Float4>(color);
  deviceResources.GetD3DDeviceContext()->ClearRenderTargetView(view, &colorFloats.x);
}

void D3D_render_step_manager::clearDepthStencil(float depth, uint8_t stencil) {
  auto& deviceResources = getDeviceResources();
  const auto depthStencil = deviceResources.GetDepthStencilView();
  deviceResources.GetD3DDeviceContext()->ClearDepthStencilView(
      depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

std::unique_ptr<oe::Render_pass> D3D_render_step_manager::createShadowMapRenderPass() {
  return std::make_unique<Render_pass_shadow>(_scene, _deviceRepository, maxRenderTargetViews());
}

void D3D_render_step_manager::beginRenderNamedEvent(const wchar_t* name) {
  getDeviceResources().PIXBeginEvent(name);
}

void D3D_render_step_manager::endRenderNamedEvent() { getDeviceResources().PIXEndEvent(); }

void D3D_render_step_manager::createRenderStepResources() {
  createRenderStepResources(_renderStep_shadowMap);
  createRenderStepResources(_renderStep_entityDeferred);
  createRenderStepResources(_renderStep_entityStandard);
  createRenderStepResources(_renderStep_debugElements);
  createRenderStepResources(_renderStep_skybox);
}

void D3D_render_step_manager::destroyRenderStepResources() {
  destroyRenderStepResources(_renderStep_shadowMap);
  destroyRenderStepResources(_renderStep_entityDeferred);
  destroyRenderStepResources(_renderStep_entityStandard);
  destroyRenderStepResources(_renderStep_debugElements);
  destroyRenderStepResources(_renderStep_skybox);
}

void D3D_render_step_manager::renderSteps(const Camera_data& cameraData) {

  auto timer = Perf_timer::start();
  renderStep(_renderStep_shadowMap, cameraData);
  timer.stop();
  _renderTimes[0] += timer.elapsedSeconds();

  timer.restart();
  renderStep(_renderStep_entityDeferred, cameraData);
  timer.stop();
  _renderTimes[1] += timer.elapsedSeconds();

  timer.restart();
  renderStep(_renderStep_entityStandard, cameraData);
  timer.stop();
  _renderTimes[2] += timer.elapsedSeconds();

  timer.restart();
  renderStep(_renderStep_debugElements, cameraData);
  timer.stop();
  _renderTimes[3] += timer.elapsedSeconds();

  timer.restart();
  renderStep(_renderStep_skybox, cameraData);
  timer.stop();
  _renderTimes[4] += timer.elapsedSeconds();
}


template <int TIdx, class TData, class... TRender_passes>
void D3D_render_step_manager::renderStep(
    Render_step<TData, TRender_passes...>& step,
    const Camera_data& cameraData) {
  if (!step.enabled)
    return;

  auto passConfig = std::get<TIdx>(step.renderPassConfigs);
  auto& pass = *std::get<TIdx>(step.renderPasses);

  auto& d3dDeviceResources = getDeviceResources();
  auto [renderTargetViews, numRenderTargets] = pass.renderTargetViewArray();
  if (numRenderTargets) {
    auto context = d3dDeviceResources.GetD3DDeviceContext();

    // TODO: Fix me!!!

    if constexpr (Render_pass_depth_mode::Disabled == passConfig.depthMode())
      context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, nullptr);
    else
      context->OMSetRenderTargets(
          static_cast<UINT>(numRenderTargets),
          renderTargetViews,
          d3dDeviceResources.GetDepthStencilView());
  }

  renderPass(passConfig, pass, cameraData);

  if constexpr (TIdx + 1 < sizeof...(TRender_passes)) {
    renderStep<TIdx + 1, TData, TRender_passes...>(step, cameraData);
  }
}

template <
    Render_pass_blend_mode TBlend_mode,
    Render_pass_depth_mode TDepth_mode,
    Render_pass_stencil_mode TStencil_mode,
    uint32_t TStencil_read_mask,
    uint32_t TStencil_write_mask>
void D3D_render_step_manager::renderPass(
    Render_pass_config<
        TBlend_mode,
        TDepth_mode,
        TStencil_mode,
        TStencil_read_mask,
        TStencil_write_mask>& renderPassInfo,
    Render_pass& renderPass,
    const Camera_data& cameraData) {
  auto& d3dDeviceResources = getDeviceResources();
  auto context = d3dDeviceResources.GetD3DDeviceContext();
  auto& commonStates = _deviceRepository->commonStates();

  // Set the blend mode
  constexpr auto opaqueSampleMask = 0xffffffff;
  constexpr std::array<float, 4> opaqueBlendFactor{0.0f, 0.0f, 0.0f, 0.0f};

  context->OMSetBlendState(renderPass.blendState(), opaqueBlendFactor.data(), opaqueSampleMask);

  // Depth/Stencil buffer mode
  context->OMSetDepthStencilState(renderPass.depthStencilState(), renderPass.stencilRef());

  // Make sure wire-frame is disabled
  context->RSSetState(commonStates.CullClockwise());

  // Set the viewport.
  auto viewport = d3dDeviceResources.GetScreenViewport();
  d3dDeviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);

  // Call the render method.
  renderPass.render(cameraData);
}

template <int TRender_pass_idx, class TData, class... TRender_passes>
void D3D_render_step_manager::createRenderStepResources(Render_step<TData, TRender_passes...>& step) {
  using Render_pass_config_type =
      typename std::tuple_element<TRender_pass_idx, decltype(step.renderPassConfigs)>::type;
  auto& pass = *std::get<TRender_pass_idx>(step.renderPasses);

  auto& commonStates = _deviceRepository->commonStates();

  // Blend state
  if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Opaque)
    pass.setBlendState(commonStates.Opaque());
  else if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Blended_alpha)
    pass.setBlendState(commonStates.AlphaBlend());
  else if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Additive)
    pass.setBlendState(commonStates.Additive());

  // Depth/Stencil
  D3D11_DEPTH_STENCIL_DESC desc = {};

  constexpr auto depthReadEnabled =
      Render_pass_config_type::depthMode() == Render_pass_depth_mode::Read_only ||
      Render_pass_config_type::depthMode() == Render_pass_depth_mode::Read_write;
  constexpr auto depthWriteEnabled =
      Render_pass_config_type::depthMode() == Render_pass_depth_mode::Write_only ||
      Render_pass_config_type::depthMode() == Render_pass_depth_mode::Read_write;

  desc.DepthEnable = depthReadEnabled || depthWriteEnabled ? TRUE : FALSE;
  desc.DepthWriteMask =
      depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  desc.DepthFunc = depthReadEnabled ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_ALWAYS;

  if constexpr (Render_pass_stencil_mode::Disabled == Render_pass_config_type::stencilMode()) {
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
  } else {
    desc.StencilEnable = TRUE;
    desc.StencilReadMask = Render_pass_config_type::stencilReadMask();
    desc.StencilWriteMask = Render_pass_config_type::stencilWriteMask();
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
  }

  desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

  desc.BackFace = desc.FrontFace;

  const auto device = _deviceRepository->deviceResources().GetD3DDevice();
  ID3D11DepthStencilState* pResult;
  ThrowIfFailed(device->CreateDepthStencilState(&desc, &pResult));

  if (pResult) {
    if constexpr (Render_pass_stencil_mode::Disabled == Render_pass_config_type::stencilMode()) {
      SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilEnabled");
    } else {
      SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilDisabled");
    }
    pass.setDepthStencilState(pResult);
    pResult->Release();

    pass.createDeviceDependentResources();
  }

  if constexpr (TRender_pass_idx + 1 < sizeof...(TRender_passes)) {
    createRenderStepResources<TRender_pass_idx + 1, TData, TRender_passes...>(step);
  }
}

template <int TRender_pass_idx, class TData, class... TRender_passes>
void D3D_render_step_manager::destroyRenderStepResources(Render_step<TData, TRender_passes...>& step) {
  auto& pass = std::get<TRender_pass_idx>(step.renderPasses);
  if (pass) {
    pass->setBlendState(nullptr);
    pass->setDepthStencilState(nullptr);
    pass->destroyDeviceDependentResources();
  }

  if constexpr (TRender_pass_idx + 1 < sizeof...(TRender_passes)) {
    destroyRenderStepResources<TRender_pass_idx + 1, TData, TRender_passes...>(step);
  }
}

oe::Viewport D3D_render_step_manager::getScreenViewport() const {
  auto d3dViewport = getDeviceResources().GetScreenViewport();
  Viewport vp;
  vp.width = d3dViewport.Width;
  vp.height = d3dViewport.Height;
  vp.maxDepth = d3dViewport.MaxDepth;
  vp.minDepth = d3dViewport.MinDepth;
  vp.topLeftX = d3dViewport.TopLeftX;
  vp.topLeftY = d3dViewport.TopLeftY;

  return vp;
}
