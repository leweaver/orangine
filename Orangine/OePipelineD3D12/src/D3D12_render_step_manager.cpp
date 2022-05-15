#include "D3D12_render_step_manager.h"
#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"

#include <OeCore/Perf_timer.h>
#include <OeCore/Render_pass_generic.h>

using namespace oe;
using namespace oe::pipeline_d3d12;

std::string D3D12_render_step_manager::_name;// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<>
void oe::create_manager(
        Manager_instance<IRender_step_manager>& out, IScene_graph_manager& sceneGraphManager,
        IDev_tools_manager& devToolsManager, D3D12_texture_manager& textureManager,
        IShadowmap_manager& shadowmapManager, IEntity_render_manager& entityRenderManager,
        ILighting_manager& lightingManager, IPrimitive_mesh_data_factory& primitiveMeshDataFactory,
        std::unique_ptr<D3D12_device_resources>& deviceResources)
{
  out = Manager_instance<IRender_step_manager>(std::make_unique<D3D12_render_step_manager>(
          sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager, lightingManager,
          primitiveMeshDataFactory, std::move(deviceResources)));
}

D3D12_render_step_manager::D3D12_render_step_manager(
        IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
        D3D12_texture_manager& textureManager, IShadowmap_manager& shadowmapManager,
        IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory, std::unique_ptr<D3D12_device_resources> deviceResources)
    : Render_step_manager(
              sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager,
              lightingManager, primitiveMeshDataFactory)
    , _deviceResources(std::move(deviceResources))
    , _textureManagerD3D12(textureManager)
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

std::unique_ptr<Render_pass> D3D12_render_step_manager::createResourceUploadBeginPass()
{
  return std::make_unique<Render_pass_generic>([dr=_deviceResources.get()](const Camera_data&, const Render_pass&) {
    dr->getResourceUploader().Begin();
  });
}

std::unique_ptr<Render_pass> D3D12_render_step_manager::createResourceUploadEndPass()
{
  return std::make_unique<Render_pass_generic>([dr=_deviceResources.get()](const Camera_data&, const Render_pass&) {
    auto future = dr->getResourceUploader().End(dr->GetCommandQueue());
    future.wait();
  });
}

void D3D12_render_step_manager::beginRenderNamedEvent(const wchar_t* name) {
  _deviceResources->PIXBeginEvent(name);
}

void D3D12_render_step_manager::endRenderNamedEvent() {
  _deviceResources->PIXEndEvent();
}

void D3D12_render_step_manager::createRenderStepResources()
{
  for (const auto& step : _renderSteps) {
    D3D12_render_step_data renderStepData{};
    for (const auto& pass : step->renderPasses) {
      renderStepData.renderPassData.push_back({});
    }
    _renderStepData.push_back(std::move(renderStepData));
  }
  _renderStepData.resize(_renderSteps.size());
}

void D3D12_render_step_manager::destroyRenderStepResources()
{
  _renderStepData.clear();
  _activeCustomRenderTargets.clear();
}

void D3D12_render_step_manager::renderSteps(const Camera_data& cameraData)
{
  _deviceResources->waitForFenceAndReset();

  beginRenderNamedEvent(L"Render");
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
  endRenderNamedEvent();

  // Make sure to unset render targets to transition custom RTV resource states from render_target to common.
  transitionToRenderTargets({});

  _deviceResources->present();
}

void D3D12_render_step_manager::transitionToRenderTargets(const std::vector<std::shared_ptr<Texture>>& textures)
{
  // Any render targets that are currently active, but not in the new set - transition out for use as SRVs
  for (const auto& oldTex : _activeCustomRenderTargets) {
    if (std::find(textures.begin(), textures.end(), oldTex) != textures.end()) {
      // Going to re-use as an RTV this pass, don't transition to SRV mode
      continue;
    }

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            _textureManagerD3D12.getResource(*oldTex),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    _deviceResources->GetPipelineCommandList()->ResourceBarrier(1, &resourceBarrier);
  }

  // Any render targets that are currently inactive, but not in the old set - transition for use as RTV
  for (const auto& texture : textures) {
    if (std::find(_activeCustomRenderTargets.begin(), _activeCustomRenderTargets.end(), texture) != _activeCustomRenderTargets.end()) {
      // Going to re-use as an RTV this pass, don't need to transition to RTV mode as it is already in it
      continue;
    }

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            _textureManagerD3D12.getResource(*texture),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    _deviceResources->GetPipelineCommandList()->ResourceBarrier(1, &resourceBarrier);
  }

  // Keep track of the state
  _activeCustomRenderTargets.assign(textures.begin(), textures.end());
}

void D3D12_render_step_manager::renderStep(
        Render_step& step, D3D12_render_step_data& renderStepData, const Camera_data& cameraData)
{
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

    auto& pass = *step.renderPasses[passIdx];
    auto& renderPassData = renderStepData.renderPassData[passIdx];
    const auto& depthStencilConfig = pass.getDepthStencilConfig();

    // Update render target views array if it is out of sync
    if (pass.popRenderTargetsChanged()) {
      const auto& renderTargets = pass.getCustomRenderTargets();
      const size_t numRenderTargets = renderTargets.size();

      for (const auto& textureIdDescriptor : renderPassData.renderTargetDescriptors) {
        const auto descriptorUsagePos = _textureIdToDescriptorRangesAndUsageCount.find(textureIdDescriptor.first);
        if (descriptorUsagePos != _textureIdToDescriptorRangesAndUsageCount.end()) {
          OE_CHECK(descriptorUsagePos->second.second > 0);
          if (--descriptorUsagePos->second.second == 0) {
            _deviceResources->getRtvHeap().releaseRange(descriptorUsagePos->second.first);
            _textureIdToDescriptorRangesAndUsageCount.erase(descriptorUsagePos);
          }
        }
      }
      renderPassData.renderTargetDescriptors.clear();
      renderPassData.cpuHandles.clear();

      for (const auto& renderTargetTexture : renderTargets) {
        TextureInternalId textureId = renderTargetTexture->internalId();
        auto& descriptorUsage = _textureIdToDescriptorRangesAndUsageCount[textureId];
        if (descriptorUsage.second == 0) {
          descriptorUsage.first = _deviceResources->getRtvHeap().allocateRange(1);
          CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorUsage.first.cpuHandle);
          ID3D12Resource* rtResource = _textureManagerD3D12.getResource(*renderTargetTexture);
          OE_CHECK(rtResource);
          _deviceResources->GetD3DDevice()->CreateRenderTargetView(rtResource, nullptr, rtvHandle);
        }
        renderPassData.renderTargetDescriptors.emplace_back(std::make_pair(textureId, descriptorUsage.first));
        renderPassData.cpuHandles.push_back(descriptorUsage.first.cpuHandle);
        ++descriptorUsage.second;
      }
    }

    beginRenderNamedEvent(L"RSM-OMSetRenderTargets");
    auto depthStencil = _deviceResources->getDepthStencil();
    if (pass.stencilRef() == 0 && !renderPassData.renderTargetDescriptors.empty()) {
      CD3DX12_CPU_DESCRIPTOR_HANDLE* depthStencilHandle = nullptr;
      if (Render_pass_depth_mode::Disabled != depthStencilConfig.getDepthMode()) {
        depthStencilHandle = &depthStencil.cpuHandle;
      }

      transitionToRenderTargets(pass.getCustomRenderTargets());

      pCommandList->OMSetRenderTargets(
              static_cast<UINT>(renderPassData.cpuHandles.size()), renderPassData.cpuHandles.data(), false,
              depthStencilHandle);
    }
    else {
      transitionToRenderTargets({});

      auto backBuffer = _deviceResources->getCurrentFrameBackBuffer();
      pCommandList->OMSetRenderTargets(1, &backBuffer.cpuHandle, false, &depthStencil.cpuHandle);
    }
    endRenderNamedEvent();

    beginRenderNamedEvent(L"RSM-Render");
    // Call the render method.
    pass.render(cameraData);
    endRenderNamedEvent();

    if (groupRenderEvents) {
      endRenderNamedEvent();
    }
  }
}

void D3D12_render_step_manager::initStatics()
{
  D3D12_render_step_manager::_name = "D3D12_render_step_manager";
}

void D3D12_render_step_manager::destroyStatics() {}
