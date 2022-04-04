#include "D3D12_render_step_manager.h"
#include "D3D12_device_resources.h"

#include <OeCore/Render_pass_generic.h>
#include <OeCore/Perf_timer.h>

using namespace oe;
using namespace oe::pipeline_d3d12;

std::string D3D12_render_step_manager::_name;// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<>
void oe::create_manager(
        Manager_instance<IRender_step_manager>& out, IScene_graph_manager& sceneGraphManager,
        IDev_tools_manager& devToolsManager, ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
        IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory, std::unique_ptr<D3D12_device_resources>& deviceResources)
{
  out = Manager_instance<IRender_step_manager>(std::make_unique<D3D12_render_step_manager>(
          sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager, lightingManager,
          primitiveMeshDataFactory, std::move(deviceResources)));
}

D3D12_render_step_manager::D3D12_render_step_manager(
        IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager, ITexture_manager& textureManager,
        IShadowmap_manager& shadowmapManager, IEntity_render_manager& entityRenderManager,
        ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        std::unique_ptr<D3D12_device_resources> deviceResources)
    : Render_step_manager(
              sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager,
              lightingManager, primitiveMeshDataFactory)
    , _deviceResources(std::move(deviceResources))
{}

// Base class overrides
void D3D12_render_step_manager::initialize()
{
  Render_step_manager::initialize();
}

void D3D12_render_step_manager::shutdown()
{
  Render_step_manager::shutdown();
}

Viewport D3D12_render_step_manager::getScreenViewport() const
{
  return {0, 0, 1, 1, 0, 1};
}

// IRender_step_manager implementation
void D3D12_render_step_manager::clearRenderTargetView(const Color& color) {}
void D3D12_render_step_manager::clearDepthStencil(float depth, uint8_t stencil) {}
std::unique_ptr<Render_pass> D3D12_render_step_manager::createShadowMapRenderPass()
{
  return std::make_unique<Render_pass_generic>([](const Camera_data&, const Render_pass&) {});
}
void D3D12_render_step_manager::beginRenderNamedEvent(const wchar_t* name) {}
void D3D12_render_step_manager::endRenderNamedEvent() {}
void D3D12_render_step_manager::createRenderStepResources() {
  _renderStepData.resize(_renderSteps.size());
}
void D3D12_render_step_manager::destroyRenderStepResources() {
  _renderStepData.clear();
}
void D3D12_render_step_manager::renderSteps(const Camera_data& cameraData)
{
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

  _deviceResources->Present();
}

void D3D12_render_step_manager::renderStep(
        Render_step& step,
        D3D12_render_step_data& renderStepData,
        const Camera_data& cameraData) {
  if (!step.enabled) {
    return;
  }

  constexpr auto opaqueSampleMask = 0xffffffff;
  constexpr std::array<float, 4> opaqueBlendFactor{0.0f, 0.0f, 0.0f, 0.0f};

  auto pCommandList = _deviceResources->GetPipelineCommandList();
  const auto viewport = &_deviceResources->GetScreenViewport();
  const auto scissorRect = CD3DX12_RECT(0, 0, static_cast<long>(viewport->Width), static_cast<long>(viewport->Height));

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

    /* Need to set the following stuff on the command list: */
    pCommandList->RSSetViewports(1, viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    auto backBuffer = _deviceResources->getCurrentFrameBackBuffer();
    auto depthStencil = _deviceResources->getDepthStencil();
    pCommandList->OMSetRenderTargets(1, &backBuffer.cpuHandle, FALSE, &depthStencil.cpuHandle);

    auto& pass = *step.renderPasses[passIdx];
    auto& renderPassData = renderStepData.renderPassData[passIdx];
    auto numRenderTargets = renderPassData.renderTargets.descriptorCount;
    const auto& depthStencilConfig = pass.getDepthStencilConfig();

    // Update render target views array if it is out of sync
    if (pass.popRenderTargetsChanged()) {
      const auto& renderTargets = pass.getRenderTargets();

      numRenderTargets = renderTargets.size();
      renderPassData.renderTargets.resize(numRenderTargets);

      for (size_t i = 0; i < numRenderTargets; ++i) {
        if (renderTargets[i]) {
          auto& rtt = D3D12_texture_manager::verifyAsD3dRenderTargetViewTexture(*renderTargets[i]);
          renderPassData._renderTargetViews[i] = rtt.renderTargetView();
          renderPassData._renderTargetViews[i]->AddRef();
        }
      }
    }
    ////beginRenderNamedEvent(L"RSM-OMSetRenderTargets");
    if (pass.stencilRef() == 0 && renderPassData._renderTargetViews.size() > 0) {
      ID3D1DepthStencilView* dsv = nullptr;
      if (Render_pass_depth_mode::Disabled != depthStencilConfig.depthMode) {
        dsv = d3dDeviceResources.GetDepthStencilView();
      }
      context->OMSetRenderTargets(
              static_cast<UINT>(numRenderTargets), renderPassData._renderTargetViews.data(), dsv);
    }
    ////endRenderNamedEvent();

    ////beginRenderNamedEvent(L"RSM-Render");
    // Call the render method.
    pass.render(cameraData);
    ////endRenderNamedEvent();

    if (groupRenderEvents) {
      endRenderNamedEvent();
    }
  }

  void D3D12_render_step_manager::initStatics()
  {
    D3D12_render_step_manager::_name = "D3D12_render_step_manager";
  }

  void D3D12_render_step_manager::destroyStatics() {}
}
