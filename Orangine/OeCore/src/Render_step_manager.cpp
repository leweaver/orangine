#include "pch.h"

#include "Render_step_manager.h"

#include <OeCore/Camera_component.h>
#include <OeCore/Clear_gbuffer_material.h>
#include <OeCore/Deferred_light_material.h>
#include <OeCore/Entity.h>
#include <OeCore/Entity_sorter.h>
#include <OeCore/IDev_tools_manager.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/Light_component.h>
#include <OeCore/Math_constants.h>
#include <OeCore/Perf_timer.h>
#include <OeCore/Render_pass_generic.h>
#include <OeCore/Render_pass_shadow.h>
#include <OeCore/Render_pass_skybox.h>
#include <OeCore/Renderable.h>
#include <OeCore/Scene.h>
#include <OeCore/Shadow_map_texture_pool.h>

#include <CommonStates.h>

using namespace DirectX;
using namespace oe;
using namespace internal;

std::string Render_step_manager::_name = "Render_step_manager";

template <> IRender_step_manager* oe::create_manager(Scene& scene)
{
  return new Render_step_manager(scene);
}

constexpr size_t g_max_render_target_views = 3;

Render_step_manager::Render_step_manager(Scene& scene)
    : IRender_step_manager(scene), _renderStep_shadowMap({}), _renderStep_entityDeferred({}),
      _renderStep_entityStandard({}), _renderStep_debugElements({}), _renderStep_skybox({}),
      _fatalError(false), _enableDeferredRendering(true)
{
  _simpleLightProvider = [this](const BoundingSphere& target, std::vector<Entity*>& lights,
                                uint32_t maxLights) {
    for (auto iter = _lightEntities->begin(); iter != _lightEntities->end(); ++iter) {
      if (lights.size() >= maxLights) {
        break;
      }
      lights.push_back(iter->get());
    }
  };
}

inline DX::DeviceResources& Render_step_manager::deviceResources() const
{
  return _scene.manager<ID3D_resources_manager>().deviceResources();
}

void Render_step_manager::initialize()
{
  using namespace std::placeholders;

  _renderableEntities =
      _scene.manager<IScene_graph_manager>().getEntityFilter({Renderable_component::type()});
  _lightEntities =
      _scene.manager<IScene_graph_manager>().getEntityFilter({Directional_light_component::type(),
                                                              Point_light_component::type(),
                                                              Ambient_light_component::type()},
                                                             Entity_filter_mode::Any);

  _alphaSorter = std::make_unique<Entity_alpha_sorter>();
  _cullSorter = std::make_unique<Entity_cull_sorter>();

  createRenderSteps();
}

void Render_step_manager::shutdown()
{
  if (_renderCount > 0) {
    std::stringstream ss;
    for (size_t i = 0; i < _renderTimes.size(); ++i) {
      const auto renderTime = _renderTimes[i];
      if (renderTime > 0.0) {
        ss << "  " << i << ": " << (1000.0 * renderTime / _renderCount) << std::endl;
      }
    }
    LOG(INFO) << "Render step average times (ms): " << std::endl << ss.str();
  }

  _renderStep_entityDeferred.data.reset();

  auto context = deviceResources().GetD3DDeviceContext();
  if (context) {
    std::array<ID3D11RenderTargetView*, g_max_render_target_views> renderTargetViews = {nullptr,
                                                                                        nullptr,
                                                                                        nullptr};
    context->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.size()),
                                renderTargetViews.data(), nullptr);
  }

  _renderStep_shadowMap.renderPasses[0].reset();
  _renderStep_entityDeferred.renderPasses[0].reset();
  _renderStep_entityDeferred.renderPasses[1].reset();
  _renderStep_entityDeferred.renderPasses[2].reset();
  _renderStep_entityStandard.renderPasses[0].reset();
  _renderStep_debugElements.renderPasses[0].reset();
  _renderStep_skybox.renderPasses[0].reset();

  _renderableEntities.reset();
  _lightEntities.reset();
}

const std::string& Render_step_manager::name() const { return _name; }

void Render_step_manager::createRenderSteps()
{
  // Shadow maps
  _renderStep_shadowMap.renderPasses[0] =
      std::make_unique<Render_pass_shadow>(_scene, g_max_render_target_views);
  _renderStep_shadowMap.renderPasses[0]->setStencilRef(1);

  // Deferred Lighting
  _renderStep_entityDeferred.renderPasses[0] =
      std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
        using Render_pass_config_type =
            std::tuple_element<0, decltype(_renderStep_entityDeferred.renderPassConfigs)>::type;
        auto& entityRenderManager = _scene.manager<IEntity_render_manager>();

        // Clear the rendered textures (ignoring depth)
        auto& d3DDeviceResources = deviceResources();
        d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step0");
        auto& quad = _renderStep_entityDeferred.data->pass0ScreenSpaceQuad;
        if (quad.rendererData && quad.material) {
          try {
            entityRenderManager.renderRenderable(quad, SSE::Matrix4::identity(), 0.0f,
                                                 Render_pass::Camera_data::IDENTITY,
                                                 Light_provider::no_light_provider,
                                                 Render_pass_config_type::blendMode(), false);
          }
          catch (std::runtime_error& e) {
            _fatalError = true;
            LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
          }
        }
        d3DDeviceResources.PIXEndEvent();
      });

  _renderStep_entityDeferred.renderPasses[1] =
      std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
        auto renderPassConfig = std::get<1>(_renderStep_entityDeferred.renderPassConfigs);

        if (_enableDeferredRendering) {
          auto& d3DDeviceResources = deviceResources();
          d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step1");

          _cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
            for (const auto& entry : entities) {
              // TODO: Stats
              // ++_renderStats.opaqueEntityCount;
              renderEntity(entry.entity, cameraData, Light_provider::no_light_provider,
                           renderPassConfig);
            }
          });

          d3DDeviceResources.PIXEndEvent();
        }
      });

  _renderStep_entityDeferred.renderPasses[2] =
      std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
        using Render_pass_config_type =
            std::tuple_element<2, decltype(_renderStep_entityDeferred.renderPassConfigs)>::type;
        const auto blendMode = Render_pass_config_type::blendMode();

        auto& d3DDeviceResources = deviceResources();
        d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_setup");

        // TODO: Clear the render target view in a more generic way.
        d3DDeviceResources.GetD3DDeviceContext()->ClearRenderTargetView(
            d3DDeviceResources.GetRenderTargetView(), DirectX::Colors::Black);

        d3DDeviceResources.PIXEndEvent();

        if (!_fatalError) {
          if (_enableDeferredRendering) {
            d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityDeferred_Step2_draw");
            renderLights(cameraData, blendMode);
            d3DDeviceResources.PIXEndEvent();
          }
        }
      });

  // Standard Lighting pass
  _renderStep_entityStandard.renderPasses[0] =
      std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
        const auto& renderPassConfig = std::get<0>(_renderStep_entityStandard.renderPassConfigs);

        _alphaSorter->waitThen([this, &renderPassConfig, &cameraData](
                                   const std::vector<Entity_alpha_sorter_entry>& entries) {
          auto& d3DDeviceResources = deviceResources();
          if (!_fatalError) {
            d3DDeviceResources.PIXBeginEvent(L"renderPass_EntityStandard_draw");

            _cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
              for (const auto& entry : entities) {
                // TODO: Stats
                // ++_renderStats.alphaEntityCount;
                renderEntity(entry.entity, cameraData, _simpleLightProvider, renderPassConfig);
              }
            });

            d3DDeviceResources.PIXEndEvent();
          }
        });
      });

  _renderStep_debugElements.renderPasses[0] =
      std::make_unique<Render_pass_generic>([this](const auto& cameraData) {
        _scene.manager<IDev_tools_manager>().renderDebugShapes(cameraData);
      });

  _renderStep_skybox.renderPasses[0] = std::make_unique<Render_pass_skybox>(_scene);
}

void Render_step_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{
  // Shadow map step resources
  _renderStep_shadowMap.data =
      std::make_shared<decltype(_renderStep_shadowMap.data)::element_type>();

  // Deferred lighting step resources
  _renderStep_entityDeferred.data =
      std::make_shared<decltype(_renderStep_entityDeferred.data)::element_type>();

  std::shared_ptr<Deferred_light_material> deferredLightMaterial;
  deferredLightMaterial.reset(new Deferred_light_material());

  _renderStep_entityDeferred.data->deferredLightMaterial = deferredLightMaterial;

  const auto& entityRenderManager = _scene.manager<IEntity_render_manager>();
  _renderStep_entityDeferred.data->pass0ScreenSpaceQuad =
      entityRenderManager.createScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
  _renderStep_entityDeferred.data->pass2ScreenSpaceQuad =
      entityRenderManager.createScreenSpaceQuad(deferredLightMaterial);

  // Debug elements lighting step resources
  _renderStep_debugElements.data =
      std::make_shared<decltype(_renderStep_debugElements.data)::element_type>();

  createRenderStepResources(_renderStep_shadowMap);
  createRenderStepResources(_renderStep_entityDeferred);
  createRenderStepResources(_renderStep_entityStandard);
  createRenderStepResources(_renderStep_debugElements);
  createRenderStepResources(_renderStep_skybox);
}

void Render_step_manager::createWindowSizeDependentResources(
    DX::DeviceResources& /*deviceResources*/, HWND /*window*/, int width, int height)
{
  auto& d3DDeviceResources = deviceResources();
  const auto d3dDevice = d3DDeviceResources.GetD3DDevice();

  // Step Deferred, Pass 0
  {
    std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets0;
    renderTargets0.resize(g_max_render_target_views);
    for (auto& renderTargetViewTexture : renderTargets0) {
      auto target = std::shared_ptr<Render_target_texture>(
          Render_target_texture::createDefaultRgb(width, height));
      target->load(d3dDevice);
      renderTargetViewTexture = std::move(target);
    }

    // Create a depth texture resource
    const auto depthTexture = std::make_shared<Depth_texture>(d3DDeviceResources);

    // Assign textures to the deferred light material
    auto deferredLightMaterial = _renderStep_entityDeferred.data->deferredLightMaterial;
    deferredLightMaterial->setColor0Texture(renderTargets0.at(0));
    deferredLightMaterial->setColor1Texture(renderTargets0.at(1));
    deferredLightMaterial->setColor2Texture(renderTargets0.at(2));
    deferredLightMaterial->setDepthTexture(depthTexture);
    deferredLightMaterial->setShadowMapDepthTexture(
        _scene.manager<IShadowmap_manager>().shadowMapDepthTextureArray());
    deferredLightMaterial->setShadowMapStencilTexture(
        _scene.manager<IShadowmap_manager>().shadowMapStencilTextureArray());

    // Give the render targets to the render pass
    std::get<0>(_renderStep_entityDeferred.renderPasses)
        ->setRenderTargets(
            std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
    std::get<1>(_renderStep_entityDeferred.renderPasses)
        ->setRenderTargets(
            std::vector<std::shared_ptr<Render_target_view_texture>>(renderTargets0));
  }

  // Step Deferred, Pass 2
  {
    std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
    renderTargets.resize(g_max_render_target_views, nullptr);

    renderTargets[0] =
        std::make_shared<Render_target_view_texture>(d3DDeviceResources.GetRenderTargetView());
    std::get<2>(_renderStep_entityDeferred.renderPasses)
        ->setRenderTargets(std::move(renderTargets));
  }

  // Step Standard, Pass 0
  {
    std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
    renderTargets.resize(g_max_render_target_views, nullptr);

    renderTargets[0] =
        std::make_shared<Render_target_view_texture>(d3DDeviceResources.GetRenderTargetView());
    std::get<0>(_renderStep_entityStandard.renderPasses)
        ->setRenderTargets(std::move(renderTargets));
  }
}

void Render_step_manager::destroyDeviceDependentResources()
{
  // Unload shadow maps
  auto& shadowMapManager = _scene.manager<IShadowmap_manager>();
  if (_lightEntities) {
    for (const auto& lightEntity : *_lightEntities) {
      // Directional light only, right now
      const auto component = lightEntity->getFirstComponentOfType<Directional_light_component>();
      if (component && component->shadowsEnabled()) {
        if (component->shadowData())
          shadowMapManager.returnTexture(std::move(component->shadowData()));
        else
          assert(!component->shadowData());
      }
    }
  }

  // Unload renderable contexts
  if (_renderableEntities) {
    for (const auto& renderableEntity : *_renderableEntities) {
      const auto component = renderableEntity->getFirstComponentOfType<Renderable_component>();
      if (component && component->materialContext()) {
        component->setMaterialContext(std::make_unique<Material_context>());
      }
    }
  }

  destroyRenderStepResources(_renderStep_shadowMap);
  destroyRenderStepResources(_renderStep_entityDeferred);
  destroyRenderStepResources(_renderStep_entityStandard);
  destroyRenderStepResources(_renderStep_debugElements);
  destroyRenderStepResources(_renderStep_skybox);

  _renderStep_shadowMap.data.reset();
  _renderStep_entityDeferred.data.reset();
  _renderStep_entityStandard.data.reset();
  _renderStep_debugElements.data.reset();
  _renderStep_skybox.data.reset();

  _fatalError = false;
}

void Render_step_manager::destroyWindowSizeDependentResources()
{
  std::array<Render_pass*, 7> renderSteps = {
      std::get<0>(_renderStep_shadowMap.renderPasses).get(),
      std::get<0>(_renderStep_entityDeferred.renderPasses).get(),
      std::get<1>(_renderStep_entityDeferred.renderPasses).get(),
      std::get<2>(_renderStep_entityDeferred.renderPasses).get(),
      std::get<0>(_renderStep_entityStandard.renderPasses).get(),
      std::get<0>(_renderStep_debugElements.renderPasses).get(),
      std::get<0>(_renderStep_skybox.renderPasses).get()};
  for (auto renderStep : renderSteps) {
    if (renderStep) {
      renderStep->clearRenderTargets();
    }
  }
}

void Render_step_manager::clearDepthStencil(float depth, uint8_t stencil) const
{
  auto& d3DDeviceResources = deviceResources();
  const auto depthStencil = d3DDeviceResources.GetDepthStencilView();
  d3DDeviceResources.GetD3DDeviceContext()->ClearDepthStencilView(
      depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void Render_step_manager::render(std::shared_ptr<Entity> cameraEntity)
{
  // Create a camera matrix
  Render_pass::Camera_data cameraData;
  SSE::Vector3 cameraPos = {};
  if (cameraEntity) {
    const auto cameraComponent = cameraEntity->getFirstComponentOfType<Camera_component>();
    if (!cameraComponent) {
      throw std::logic_error("Camera entity must have a Camera_component");
    }

    if (_fatalError)
      return;

    cameraData = createCameraData(*cameraComponent);
    cameraPos = cameraEntity->worldPosition();
  }
  else {
    cameraData =
        createCameraData(SSE::Matrix4::identity(), Camera_component::DEFAULT_FOV,
                         Camera_component::DEFAULT_NEAR_PLANE, Camera_component::DEFAULT_FAR_PLANE);
  }
  const auto frustum = BoundingFrustumRH(cameraData.projectionMatrix);

  _cullSorter->beginSortAsync(_renderableEntities->begin(), _renderableEntities->end(), frustum);

  // Block on the cull sorter, since we can't render until it is done; and it is a good place to
  // kick off the alpha sort.
  _cullSorter->waitThen(
      [this, cameraPos, cameraData](const std::vector<Entity_cull_sorter_entry>& entities) {
        // Find the list of entities that have alpha, and sort them.
        _alphaSorter->beginSortAsync(entities.begin(), entities.end(), cameraPos);

        // Render steps
        _scene.manager<IEntity_render_manager>().clearRenderStats();

        clearDepthStencil();

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
      });

  ++_renderCount;
  if (_renderCount == 1) {
    std::fill(_renderTimes.begin(), _renderTimes.end(), 0.0);
  }
}

// TODO: Move this to a Render_pass_deferred class?
void Render_step_manager::renderLights(const Render_pass::Camera_data& cameraData,
                                       Render_pass_blend_mode blendMode)
{
  try {
    auto& quad = _renderStep_entityDeferred.data->pass2ScreenSpaceQuad;
    auto& entityRenderManager = _scene.manager<IEntity_render_manager>();

    const auto deferredLightMaterial = _renderStep_entityDeferred.data->deferredLightMaterial;
    assert(deferredLightMaterial == quad.material);

    std::vector<Entity*> deferredLights;
    const auto deferredLightProvider = [&deferredLights](const BoundingSphere& target,
                                                         std::vector<Entity*>& lights,
                                                         uint32_t maxLights) {
      if (lights.size() + deferredLights.size() > static_cast<size_t>(maxLights)) {
        throw std::logic_error("destination lights array is not large enough to contain "
                               "Deferred_light_material's lights");
      }

      lights.insert(lights.end(), deferredLights.begin(), deferredLights.end());
    };

    auto lightIndex = 0u;
    const auto maxLights = deferredLightMaterial->max_lights;
    auto renderedOnce = false;

    deferredLightMaterial->setupEmitted(true);
    for (auto eIter = _lightEntities->begin(); eIter != _lightEntities->end();
         ++eIter, ++lightIndex) {
      deferredLights.push_back(eIter->get());

      if (lightIndex == maxLights) {
        entityRenderManager.renderRenderable(quad, SSE::Matrix4::identity(), 0.0f, cameraData,
                                             deferredLightProvider, blendMode, false);

        deferredLightMaterial->setupEmitted(false);
        renderedOnce = true;
        deferredLights.clear();

        lightIndex = 0;
      }
    }

    if (!deferredLights.empty() || !renderedOnce) {
      entityRenderManager.renderRenderable(quad, SSE::Matrix4::identity(), 0.0f, cameraData,
                                           deferredLightProvider, blendMode, false);
    }
  }
  catch (const std::runtime_error& e) {
    _fatalError = true;
    LOG(WARNING) << "Failed to render lights.\n" << e.what();
  }
}

Render_pass::Camera_data Render_step_manager::createCameraData(Camera_component& component) const
{
  return createCameraData(component.entity().worldTransform(), component.fov(),
                          component.nearPlane(), component.farPlane());
}

Render_pass::Camera_data Render_step_manager::createCameraData(const SSE::Matrix4& worldTransform,
                                                               float fov, float nearPlane,
                                                               float farPlane) const
{
  const auto viewport = deviceResources().GetScreenViewport();
  const auto aspectRatio = viewport.Width / viewport.Height;

  // Construct camera axis vectors, to create a view matrix using lookAt.
  const auto pos = SSE::Point3(worldTransform.getTranslation());
  const auto forward = worldTransform * math::forward;
  const auto up = worldTransform * math::up;

  // This optimization, while fancy, breaks our ability to read in the world pos from the depth
  // buffer in deferred lighting.
  // auto invFarPlane = component.farPlane() != 0.0f ? 1.0f / component.farPlane() : 0.0f;
  //_cameraData.projectionMatrix._33 *= invFarPlane;
  //_cameraData.projectionMatrix._43 *= invFarPlane;

  const auto perspectiveMat = SSE::Matrix4::perspective(fov, aspectRatio, nearPlane, farPlane);

  return {SSE::Matrix4::lookAt(pos, pos + forward.getXYZ(), up.getXYZ()), perspectiveMat, fov,
          aspectRatio};
}

template <int TRender_pass_idx, class TData, class... TRender_passes>
void Render_step_manager::createRenderStepResources(Render_step<TData, TRender_passes...>& step)
{
  using Render_pass_config_type = typename std::tuple_element<TRender_pass_idx, decltype(step.renderPassConfigs)>::type;
  auto& pass = *std::get<TRender_pass_idx>(step.renderPasses);

  auto& d3DResourcesManager = _scene.manager<ID3D_resources_manager>();
  auto& commonStates = d3DResourcesManager.commonStates();

  // Blend state
  if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Opaque)
    pass.setBlendState(commonStates.Opaque());
  else if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Blended_Alpha)
    pass.setBlendState(commonStates.AlphaBlend());
  else if constexpr (Render_pass_config_type::blendMode() == Render_pass_blend_mode::Additive)
    pass.setBlendState(commonStates.Additive());

  // Depth/Stencil
  D3D11_DEPTH_STENCIL_DESC desc = {};

  constexpr auto depthReadEnabled = Render_pass_config_type::depthMode() == Render_pass_depth_mode::ReadOnly ||
                                    Render_pass_config_type::depthMode() == Render_pass_depth_mode::ReadWrite;
  constexpr auto depthWriteEnabled = Render_pass_config_type::depthMode() == Render_pass_depth_mode::WriteOnly ||
                                     Render_pass_config_type::depthMode() == Render_pass_depth_mode::ReadWrite;

  desc.DepthEnable = depthReadEnabled || depthWriteEnabled ? TRUE : FALSE;
  desc.DepthWriteMask =
      depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  desc.DepthFunc = depthReadEnabled ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_ALWAYS;

  if constexpr (Render_pass_stencil_mode::Disabled == Render_pass_config_type::stencilMode()) {
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
  }
  else {
    desc.StencilEnable = TRUE;
    desc.StencilReadMask = Render_pass_config_type::stencilReadMask();
    desc.StencilWriteMask = Render_pass_config_type::stencilWriteMask();
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
  }

  desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

  desc.BackFace = desc.FrontFace;

  const auto device = d3DResourcesManager.deviceResources().GetD3DDevice();
  ID3D11DepthStencilState* pResult;
  ThrowIfFailed(device->CreateDepthStencilState(&desc, &pResult));

  if (pResult) {
    if constexpr (Render_pass_stencil_mode::Disabled == Render_pass_config_type::stencilMode()) {
      SetDebugObjectName(pResult, "Render_step_manager:DepthStencil:StencilEnabled");
    }
    else {
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
void Render_step_manager::destroyRenderStepResources(Render_step<TData, TRender_passes...>& step)
{
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

template <int TIdx, class TData, class... TRender_passes>
void Render_step_manager::renderStep(Render_step<TData, TRender_passes...>& step,
                                     const Render_pass::Camera_data& cameraData)
{
  if (!step.enabled)
    return;

  auto passConfig = std::get<TIdx>(step.renderPassConfigs);
  auto& pass = *std::get<TIdx>(step.renderPasses);

  auto& d3DDeviceResources = deviceResources();
  auto [renderTargetViews, numRenderTargets] = pass.renderTargetViewArray();
  if (numRenderTargets) {
    auto context = d3DDeviceResources.GetD3DDeviceContext();

    // TODO: Fix me!!!

    if constexpr (Render_pass_depth_mode::Disabled == passConfig.depthMode())
      context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews, nullptr);
    else
      context->OMSetRenderTargets(static_cast<UINT>(numRenderTargets), renderTargetViews,
                                  d3DDeviceResources.GetDepthStencilView());
  }

  renderPass(passConfig, pass, cameraData);

  if constexpr (TIdx + 1 < sizeof...(TRender_passes)) {
    renderStep<TIdx + 1, TData, TRender_passes...>(step, cameraData);
  }
}

template <Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode,
          Render_pass_stencil_mode TStencil_mode, uint32_t TStencil_read_mask,
          uint32_t TStencil_write_mask>
void Render_step_manager::renderPass(
    Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask,
                       TStencil_write_mask>& renderPassInfo,
    Render_pass& renderPass, const Render_pass::Camera_data& cameraData)
{
  auto context = deviceResources().GetD3DDeviceContext();
  auto& commonStates = _scene.manager<ID3D_resources_manager>().commonStates();

  // Set the blend mode
  constexpr auto opaqueSampleMask = 0xffffffff;
  constexpr std::array<float, 4> opaqueBlendFactor{0.0f, 0.0f, 0.0f, 0.0f};

  context->OMSetBlendState(renderPass.blendState(), opaqueBlendFactor.data(), opaqueSampleMask);

  // Depth/Stencil buffer mode
  context->OMSetDepthStencilState(renderPass.depthStencilState(), renderPass.stencilRef());

  // Make sure wire-frame is disabled
  context->RSSetState(commonStates.CullClockwise());

  // Set the viewport.
  auto& d3DDeviceResources = deviceResources();
  auto viewport = d3DDeviceResources.GetScreenViewport();
  d3DDeviceResources.GetD3DDeviceContext()->RSSetViewports(1, &viewport);

  // Call the render method.
  renderPass.render(cameraData);
}

template <Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode,
          Render_pass_stencil_mode TStencil_mode, uint32_t TStencilReadMask,
          uint32_t TStencilWriteMask>
void Render_step_manager::renderEntity(
    Entity* entity, const Render_pass::Camera_data& cameraData,
    Light_provider::Callback_type& lightProvider,
    const Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencilReadMask,
                             TStencilWriteMask>& renderPassInfo)
{
  const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
  if (!renderable || !renderable->visible())
    return;

  const auto material = renderable->material().get();
  assert(material != nullptr);

  // Check that this render pass supports this materials alpha mode
  if constexpr (TBlend_mode == Render_pass_blend_mode::Blended_Alpha) {
    if (material->getAlphaMode() != Material_alpha_mode::Blend)
      return;
  }
  else {
    if (material->getAlphaMode() == Material_alpha_mode::Blend)
      return;
  }

  _scene.manager<IEntity_render_manager>().renderEntity(*renderable, cameraData, lightProvider,
                                                        TBlend_mode);
}
