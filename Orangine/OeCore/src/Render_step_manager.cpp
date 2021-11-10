#include "pch.h"

#include <OeCore/Render_step_manager.h>

#include <OeCore/Camera_component.h>
#include <OeCore/Clear_gbuffer_material.h>
#include <OeCore/Light_component.h>
#include <OeCore/Render_pass_generic.h>
#include <OeCore/Render_pass_skybox.h>

using namespace oe;

std::string Render_step_manager::_name = "Render_step_manager";

Render_step_manager::Render_step::Render_step(
    std::unique_ptr<Render_pass>&& renderPass,
    std::wstring name)
    : name(std::move(name)) {
  renderPasses.emplace_back(std::move(renderPass));
}

Render_step_manager::Render_step_manager(
        IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager, ITexture_manager& textureManager,
        IShadowmap_manager& shadowmapManager, IEntity_render_manager& entityRenderManager,
        ILighting_manager& lightingManager)
    : IRender_step_manager()
    , Manager_base()
    , Manager_deviceDependent()
    , Manager_windowDependent()
    , _fatalError(false)
    , _enableDeferredRendering(true)
    , _sceneGraphManager(sceneGraphManager)
    , _devToolsManager(devToolsManager)
    , _textureManager(textureManager)
    , _shadowmapManager(shadowmapManager)
    , _entityRenderManager(entityRenderManager)
    , _lightingManager(lightingManager)
{
  _simpleLightProvider = [this](const BoundingSphere& target, std::vector<Entity*>& lights, uint32_t maxLights) {
    for (auto iter = _lightEntities->begin(); iter != _lightEntities->end(); ++iter) {
      if (lights.size() >= maxLights) {
        break;
      }
      lights.push_back(iter->get());
    }
  };
}

void Render_step_manager::initialize() {
  using namespace std::placeholders;

  _renderableEntities = _sceneGraphManager.getEntityFilter({Renderable_component::type()});
  _lightEntities = _sceneGraphManager.getEntityFilter(
      {Directional_light_component::type(),
       Point_light_component::type(),
       Ambient_light_component::type()},
      Entity_filter_mode::Any);

  _alphaSorter = std::make_unique<Entity_alpha_sorter>();
  _cullSorter = std::make_unique<Entity_cull_sorter>();

  createRenderSteps();
}

void Render_step_manager::shutdown() {
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

  _renderSteps.clear();

  _renderableEntities.reset();
  _lightEntities.reset();
}

const std::string& Render_step_manager::name() const { return _name; }

void Render_step_manager::createRenderSteps() {
  // Shadow maps
  {
    auto pass = createShadowMapRenderPass();
    auto stencilConfig = Depth_stencil_config(Default_values());
    stencilConfig.stencilMode = Render_pass_stencil_mode::Enabled;
    pass->setDepthStencilConfig(stencilConfig);
    pass->setStencilRef(1);

    _renderSteps.emplace_back(new Render_step(std::move(pass), L"Shadow Map"));
  }

  // Deferred Lighting
  {
    auto clearGBufferPass = std::make_unique<Render_pass_generic>(
        [this](const auto& cameraData, const Render_pass& pass) {
          // Clear the rendered textures (ignoring depth)
          auto& quad = _renderPassDeferredData.pass0ScreenSpaceQuad;
          if (!quad.rendererData.expired() && quad.material) {
            try {
              _entityRenderManager.renderRenderable(
                  quad,
                  SSE::Matrix4::identity(),
                  0.0f,
                  Camera_data::IDENTITY,
                  Light_provider::no_light_provider,
                  pass.getDepthStencilConfig().blendMode,
                  false);
            } catch (std::runtime_error& e) {
              _fatalError = true;
              LOG(WARNING) << "Failed to clear G Buffer.\n" << e.what();
            }
          }
        });

    auto drawEntitiesPass = std::make_unique<Render_pass_generic>(
        [this](const auto& cameraData, const Render_pass& pass) {
          if (_enableDeferredRendering) {
            _cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
              for (const auto& entry : entities) {
                // TODO: Stats
                // ++_renderStats.opaqueEntityCount;
                renderEntity(entry.entity, cameraData, Light_provider::no_light_provider, pass.getDepthStencilConfig());
              }
            });
          }
        });

    auto drawLightsPass = std::make_unique<Render_pass_generic>(
        [this](const auto& cameraData, const Render_pass& pass) {
          clearRenderTargetView(oe::Colors::Black);

          if (!_fatalError) {
            if (_enableDeferredRendering) {
              renderLights(cameraData, pass.getDepthStencilConfig().blendMode);
            }
          }
        });

    clearGBufferPass->setDepthStencilConfig(
        Depth_stencil_config(Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled));
    drawEntitiesPass->setDepthStencilConfig(
        Depth_stencil_config(Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Read_write));
    drawLightsPass->setDepthStencilConfig(
        Depth_stencil_config(Render_pass_blend_mode::Additive, Render_pass_depth_mode::Disabled));

    clearGBufferPass->setDrawDestination(Render_pass_destination::Gbuffer);
    drawEntitiesPass->setDrawDestination(Render_pass_destination::Gbuffer);
    drawLightsPass->setDrawDestination(Render_pass_destination::Render_target_view);

    auto step = std::make_unique<Render_step>(L"Deferred Lighting");

    step->renderPasses.emplace_back(std::move(clearGBufferPass));
    step->renderPasses.emplace_back(std::move(drawEntitiesPass));
    step->renderPasses.emplace_back(std::move(drawLightsPass));
    _renderSteps.emplace_back(std::move(step));
  }

  // Standard Lighting pass
  {
    auto drawTransparentEntitiesPass = std::make_unique<Render_pass_generic>(
        [this](const auto& cameraData, const Render_pass& pass) {
          _alphaSorter->waitThen([this, &cameraData, &pass](
                                     const std::vector<Entity_alpha_sorter_entry>& entries) {
            if (!_fatalError) {
              _cullSorter->waitThen([&](const std::vector<Entity_cull_sorter_entry>& entities) {
                for (const auto& entry : entities) {
                  // TODO: Stats
                  // ++_renderStats.alphaEntityCount;
                  renderEntity(entry.entity, cameraData, _simpleLightProvider, pass.getDepthStencilConfig());
                }
              });
            }
          });
        });
    drawTransparentEntitiesPass->setDepthStencilConfig(Depth_stencil_config(
        Render_pass_blend_mode::Blended_alpha, Render_pass_depth_mode::Read_write));

    drawTransparentEntitiesPass->setDrawDestination(Render_pass_destination::Render_target_view);

    _renderSteps.emplace_back(new Render_step(std::move(drawTransparentEntitiesPass), L"Standard Lighting"));
  }

  // Debug Elements
  {
    auto drawDebugElementsPass = std::make_unique<Render_pass_generic>(
        [this](const auto& cameraData, const Render_pass& pass) {
          _devToolsManager.renderDebugShapes(cameraData);
        });
    drawDebugElementsPass->setDepthStencilConfig(
        Depth_stencil_config(Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Write_only));
    drawDebugElementsPass->setDrawDestination(Render_pass_destination::Render_target_view);
    _renderSteps.emplace_back(new Render_step(std::move(drawDebugElementsPass), L"Debug Elements"));
  }

  // Sky box
  {
    auto drawSkyboxPass = std::make_unique<Render_pass_skybox>(_entityRenderManager, _lightingManager);
    drawSkyboxPass->setDepthStencilConfig(
        Depth_stencil_config(Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Read_only));
    drawSkyboxPass->setDrawDestination(Render_pass_destination::Render_target_view);
    _renderSteps.emplace_back(new Render_step(std::move(drawSkyboxPass), L"Sky box"));
  }
}

void Render_step_manager::createDeviceDependentResources() {
  auto deferredLightMaterial = std::make_shared<Deferred_light_material>();
  _renderPassDeferredData.deferredLightMaterial = deferredLightMaterial;

  _renderPassDeferredData.pass0ScreenSpaceQuad =
      _entityRenderManager.createScreenSpaceQuad(std::make_shared<Clear_gbuffer_material>());
  _renderPassDeferredData.pass2ScreenSpaceQuad =
      _entityRenderManager.createScreenSpaceQuad(deferredLightMaterial);

  createRenderStepResources();
}

void Render_step_manager::createWindowSizeDependentResources(
    HWND /*window*/,
    int width,
    int height) {
  std::vector<std::shared_ptr<Texture>> renderTargets0;
  renderTargets0.resize(maxRenderTargetViews());
  for (auto& renderTargetViewTexture : renderTargets0) {
    const auto renderTargetTexture = _textureManager.createRenderTargetTexture(width, height);
    _textureManager.load(*renderTargetTexture);
    renderTargetViewTexture = renderTargetTexture;
  }

  // Step Deferred, Pass 0
  {

    // Create a depth texture resource
    const auto depthTexture = _textureManager.createDepthTexture();

    // Assign textures to the deferred light material
    auto deferredLightMaterial = _renderPassDeferredData.deferredLightMaterial;
    deferredLightMaterial->setColor0Texture(renderTargets0.at(0));
    deferredLightMaterial->setColor1Texture(renderTargets0.at(1));
    deferredLightMaterial->setColor2Texture(renderTargets0.at(2));
    deferredLightMaterial->setDepthTexture(depthTexture);
    deferredLightMaterial->setShadowMapDepthTexture(_shadowmapManager.shadowMapDepthTextureArray());
    deferredLightMaterial->setShadowMapStencilTexture(_shadowmapManager.shadowMapStencilTextureArray());
  }

  const auto renderTargetViewTexture = _textureManager.createRenderTargetViewTexture();
  std::vector<std::shared_ptr<Texture>> renderTargets1;
  renderTargets1.resize(maxRenderTargetViews(), nullptr);
  renderTargets1[0] = renderTargetViewTexture;

  for (auto& step : _renderSteps) {
    for (auto& pass : step->renderPasses) {
      switch (pass->getDrawDestination()) {
      case Render_pass_destination::Gbuffer:
        // Give the render targets to the render pass
        pass->setRenderTargets(renderTargets0);
        break;
      case Render_pass_destination::Render_target_view:
        // Same as default
      default:
        // Give the render targets to the render pass
        pass->setRenderTargets(renderTargets1);
        break;
      }
    }
  }
}

void Render_step_manager::destroyDeviceDependentResources() {
  // Unload shadow maps
  if (_lightEntities) {
    for (const auto& lightEntity : *_lightEntities) {
      // Directional light only, right now
      auto* const component = lightEntity->getFirstComponentOfType<Directional_light_component>();
      if (component && component->shadowsEnabled()) {
        if (component->shadowData()) {
          _shadowmapManager.returnTexture(std::move(component->shadowData()->shadowMap));
        }
        else {
          assert(!component->shadowData());
        }
      }
    }
  }

  destroyRenderStepResources();

  _renderPassDeferredData = {};

  _fatalError = false;
}

void Render_step_manager::destroyWindowSizeDependentResources() {
  for (auto& step : _renderSteps) {
    for (auto& pass : step->renderPasses) {
      pass->clearRenderTargets();
    }
  }
}

void Render_step_manager::setCameraEntity(std::shared_ptr<Entity> cameraEntity)
{
  _cameraEntity = cameraEntity;
}

void Render_step_manager::render() {
  // Create a camera matrix
  Camera_data cameraData;
  SSE::Vector3 cameraPos = {};
  if (_cameraEntity) {
    const auto cameraComponent = _cameraEntity->getFirstComponentOfType<Camera_component>();
    if (!cameraComponent) {
      OE_THROW(std::logic_error("Camera entity must have a Camera_component"));
    }

    if (_fatalError) {
      return;
    }

    cameraData = createCameraData(*cameraComponent);
    cameraPos = _cameraEntity->worldPosition();
  } else {
    cameraData = createCameraData(
        SSE::Matrix4::identity(),
        Camera_component::DEFAULT_FOV,
        Camera_component::DEFAULT_NEAR_PLANE,
        Camera_component::DEFAULT_FAR_PLANE);
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
        _entityRenderManager.clearRenderStats();

        clearDepthStencil(1.0f, 0);

        // Load lighting for camera
        applyEnvironmentVolume(cameraPos);

        renderSteps(cameraData);
      });

  ++_renderCount;

  // Erase the first frame render timings as they will be massively spiked.
  if (_renderCount == 1) {
    std::fill(_renderTimes.begin(), _renderTimes.end(), 0.0);
  }
}

void Render_step_manager::applyEnvironmentVolume(const Vector3& cameraPos) {
  _lightingManager.setCurrentVolumeEnvironmentLighting(cameraPos);
}

// TODO: Move this to a Render_pass_deferred class?
void Render_step_manager::renderLights(
    const Camera_data& cameraData,
    Render_pass_blend_mode blendMode) {
  try {
    auto& quad = _renderPassDeferredData.pass2ScreenSpaceQuad;

    const auto deferredLightMaterial = _renderPassDeferredData.deferredLightMaterial;
    assert(deferredLightMaterial == quad.material);

    std::vector<Entity*> deferredLights;
    const auto funcName = static_cast<const char*>(__PRETTY_FUNCTION__);
    const auto deferredLightProvider =
        [&deferredLights, funcName](
            const BoundingSphere& target, std::vector<Entity*>& lights, uint32_t maxLights) {
          if (lights.size() + deferredLights.size() > static_cast<size_t>(maxLights)) {
            throw oe::log_exception_for_throw(
                    std::logic_error("destination lights array is not large enough to contain "
                                     "Deferred_light_material's lights"),
                    __FILE__, __LINE__, funcName);
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
        _entityRenderManager.renderRenderable(
            quad,
            SSE::Matrix4::identity(),
            0.0f,
            cameraData,
            deferredLightProvider,
            blendMode,
            false);

        deferredLightMaterial->setupEmitted(false);
        renderedOnce = true;
        deferredLights.clear();

        lightIndex = 0;
      }
    }

    if (!deferredLights.empty() || !renderedOnce) {
      _entityRenderManager.renderRenderable(
          quad,
          SSE::Matrix4::identity(),
          0.0f,
          cameraData,
          deferredLightProvider,
          blendMode,
          false);
    }
  } catch (const std::runtime_error& e) {
    _fatalError = true;
    LOG(WARNING) << "Failed to render lights.\n" << e.what();
  }
}

Camera_data Render_step_manager::createCameraData(Camera_component& component) const {
  return createCameraData(
          component.getEntity().worldTransform(),
      component.fov(),
      component.nearPlane(),
      component.farPlane());
}

Camera_data Render_step_manager::createCameraData(
    const SSE::Matrix4& worldTransform,
    float fov,
    float nearPlane,
    float farPlane) const {
  const auto viewport = getScreenViewport();
  const auto aspectRatio = viewport.width / viewport.height;

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

  return {
      SSE::Matrix4::lookAt(pos, pos + forward.getXYZ(), up.getXYZ()),
      perspectiveMat,
      fov,
      aspectRatio};
}

void Render_step_manager::renderEntity(
    Entity* entity,
    const Camera_data& cameraData,
    Light_provider::Callback_type& lightProvider,
    const Depth_stencil_config& depthStencilConfig) const {
  auto* const renderable = entity->getFirstComponentOfType<Renderable_component>();
  if (!renderable || !renderable->visible()) {
    return;
  }

  auto* const material = renderable->material().get();
  assert(material != nullptr);

  // Check that this render pass supports this materials alpha mode
  if (depthStencilConfig.blendMode == Render_pass_blend_mode::Blended_alpha) {
    if (material->getAlphaMode() != Material_alpha_mode::Blend) {
      return;
    }
  } else {
    if (material->getAlphaMode() == Material_alpha_mode::Blend) {
      return;
    }
  }

  _entityRenderManager.renderEntity(*renderable, cameraData, lightProvider, depthStencilConfig.blendMode);
}

BoundingFrustumRH Render_step_manager::createFrustum(const Camera_component& cameraComponent) {
  const auto viewport = getScreenViewport();
  const auto aspectRatio = viewport.width / viewport.height;

  const auto projMatrix = SSE::Matrix4::perspective(
          cameraComponent.fov(), aspectRatio, cameraComponent.nearPlane(), cameraComponent.farPlane());

  auto frustum = oe::BoundingFrustumRH(projMatrix);
  const auto& entity = cameraComponent.getEntity();
  frustum.origin = entity.worldPosition();
  frustum.orientation = entity.worldRotation();

  return frustum;
}