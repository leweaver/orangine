#include "pch.h"

#include "D3D_device_repository.h"
#include "D3D_render_pass_shadow.h"
#include "D3D_render_step_manager.h"


#include "D3D_texture_manager.h"
#include "OeCore/Perf_timer.h"

#include <OeCore/Color.h>
#include <OeCore/Render_pass_generic.h>

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
  _renderStepData.resize(_renderSteps.size(), {});
  for (size_t i = 0; i < _renderSteps.size(); ++i) {
    createRenderStepResources(*_renderSteps[i], _renderStepData[i]);
  }
}

void D3D_render_step_manager::destroyRenderStepResources() {
  for (size_t i = 0; i < _renderSteps.size(); ++i) {
    destroyRenderStepResources(*_renderSteps[i], _renderStepData[i]);
  }
  _renderStepData.clear();
}

void D3D_render_step_manager::renderSteps(const Camera_data& cameraData) {
  auto timer = Perf_timer::start();
  for (size_t i = 0; i < _renderSteps.size(); ++i) {
    timer.restart();
    auto& step = *_renderSteps[i];
    beginRenderNamedEvent(step.name.c_str());
    renderStep(step, _renderStepData[i], cameraData);
    endRenderNamedEvent();
    timer.stop();
    _renderTimes[i] += timer.elapsedSeconds();
  }
}

void D3D_render_step_manager::renderStep(
    Render_step& step,
    D3D_render_step_data& renderStepData,
    const Camera_data& cameraData) {
  if (!step.enabled)
    return;

  auto& d3dDeviceResources = getDeviceResources();
  auto* const context = d3dDeviceResources.GetD3DDeviceContext();
  auto& commonStates = _deviceRepository->commonStates();

  constexpr auto opaqueSampleMask = 0xffffffff;
  constexpr std::array<float, 4> opaqueBlendFactor{0.0f, 0.0f, 0.0f, 0.0f};

  bool groupRenderEvents = step.renderPasses.size() > 1;
  for (auto passIdx = 0; passIdx < step.renderPasses.size(); ++passIdx) {
    if (groupRenderEvents) {
      if (_passNames.size() == passIdx) {
        std::wstringstream ss;
        ss << L"Pass " << passIdx;
        _passNames.push_back(ss.str());
      }
      beginRenderNamedEvent(_passNames[passIdx].c_str());
    }

    auto& pass = *step.renderPasses[passIdx];
    auto& renderPassData = renderStepData.renderPassData[passIdx];
    auto numRenderTargets = renderPassData._renderTargetViews.size();
    const auto& depthStencilConfig = pass.getDepthStencilConfig();

    // Update render target views array if it is out of sync
    if (pass.popRenderTargetsChanged()) {
      const auto& renderTargets = pass.getRenderTargets();
      for (auto& renderTargetView : renderPassData._renderTargetViews) {
        if (renderTargetView) {
          renderTargetView->Release();
          renderTargetView = nullptr;
        }
      }

      numRenderTargets = renderTargets.size();
      renderPassData._renderTargetViews.resize(numRenderTargets, nullptr);

      for (size_t i = 0; i < renderPassData._renderTargetViews.size(); ++i) {
        if (renderTargets[i]) {
          auto& rtt = D3D_texture_manager::verifyAsD3dRenderTargetViewTexture(*renderTargets[i]);
          renderPassData._renderTargetViews[i] = rtt.renderTargetView();
          renderPassData._renderTargetViews[i]->AddRef();
        }
      }
    }
    ////beginRenderNamedEvent(L"RSM-OMSetRenderTargets");
    if (pass.stencilRef() == 0 && renderPassData._renderTargetViews.size() > 0) {
      ID3D11DepthStencilView* dsv = nullptr;
      if (Render_pass_depth_mode::Disabled != depthStencilConfig.depthMode) {
        dsv = d3dDeviceResources.GetDepthStencilView();
      }
      context->OMSetRenderTargets(
          static_cast<UINT>(numRenderTargets), renderPassData._renderTargetViews.data(), dsv);
    }
    ////endRenderNamedEvent();

    ////beginRenderNamedEvent(L"RSM-OMSetBlendStateEtc");
    // Set the blend mode
    context->OMSetBlendState(
        renderPassData._blendState.Get(), opaqueBlendFactor.data(), opaqueSampleMask);

    // Depth/Stencil buffer mode
    context->OMSetDepthStencilState(renderPassData._depthStencilState.Get(), pass.stencilRef());

    // Make sure wire-frame is disabled
    context->RSSetState(commonStates.CullClockwise());

    // Set the viewport.
    auto viewport = d3dDeviceResources.GetScreenViewport();
    d3dDeviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);
    ////endRenderNamedEvent();

    ////beginRenderNamedEvent(L"RSM-Render");
    // Call the render method.
    pass.render(cameraData);
    ////endRenderNamedEvent();

    if (groupRenderEvents) {
      endRenderNamedEvent();
    }
  }
}

void D3D_render_step_manager::createRenderStepResources(
    Render_step& step,
    D3D_render_step_data& renderStepData) {
  auto& commonStates = _deviceRepository->commonStates();
  renderStepData.renderPassData.resize(step.renderPasses.size());
  for (size_t i = 0; i < step.renderPasses.size(); ++i) {
    auto* const pass = step.renderPasses[i].get();
    if (!pass) {
      continue;
    }
    auto& renderPassData = renderStepData.renderPassData[i];

    const auto& depthStencilConfig = pass->getDepthStencilConfig();

    // Blend state
    if (depthStencilConfig.blendMode == Render_pass_blend_mode::Opaque)
      renderPassData._blendState = commonStates.Opaque();
    else if (depthStencilConfig.blendMode == Render_pass_blend_mode::Blended_alpha)
      renderPassData._blendState = commonStates.AlphaBlend();
    else if (depthStencilConfig.blendMode == Render_pass_blend_mode::Additive)
      renderPassData._blendState = commonStates.Additive();

    // Depth/Stencil
    D3D11_DEPTH_STENCIL_DESC desc = {};

    const auto depthReadEnabled =
        depthStencilConfig.depthMode == Render_pass_depth_mode::Read_only ||
        depthStencilConfig.depthMode == Render_pass_depth_mode::Read_write;
    const auto depthWriteEnabled =
        depthStencilConfig.depthMode == Render_pass_depth_mode::Write_only ||
        depthStencilConfig.depthMode == Render_pass_depth_mode::Read_write;

    desc.DepthEnable = depthReadEnabled || depthWriteEnabled ? TRUE : FALSE;
    desc.DepthWriteMask =
        depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = depthReadEnabled ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_ALWAYS;

    if (Render_pass_stencil_mode::Disabled == depthStencilConfig.stencilMode) {
      desc.StencilEnable = FALSE;
      desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
      desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    } else {
      desc.StencilEnable = TRUE;
      desc.StencilReadMask = depthStencilConfig.stencilReadMask;
      desc.StencilWriteMask = depthStencilConfig.stencilWriteMask;
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
      if (Render_pass_stencil_mode::Disabled == depthStencilConfig.stencilMode) {
        SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilDisabled");
      } else {
        SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilEnabled");
      }
      renderPassData._depthStencilState = pResult;
      pResult->Release();
    }
  }
}

void D3D_render_step_manager::destroyRenderStepResources(
    Render_step& step,
    D3D_render_step_data& renderStepData) {
  renderStepData.renderPassData.clear();
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
