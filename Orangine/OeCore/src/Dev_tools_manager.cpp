#include "pch.h"

#include "Dev_tools_manager.h"

#include "OeCore/Animation_controller_component.h"
#include "OeCore/Fps_counter.h"
#include "OeCore/Math_constants.h"
#include "OeCore/Primitive_mesh_data_factory.h"
#include "OeCore/Renderable.h"
#include "OeCore/Renderer_data.h"
#include "OeCore/Scene.h"
#include "OeCore/Skinned_mesh_component.h"
#include "OeCore/Unlit_material.h"
#include "OeCore/VectorLog.h"
#include "OeCore/IEntity_render_manager.h"
#include "OeCore/IMaterial_manager.h"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <algorithm>

using namespace DirectX;
using namespace oe;
using namespace internal;

const auto g_hashSeed_sphere = std::hash<std::string>{}("sphere");
const auto g_hashSeed_cone = std::hash<std::string>{}("cone");
const auto g_hashSeed_boundingBox = std::hash<std::string>{}("boundingBox");
const auto g_hashSeed_axisWidget = std::hash<std::string>{}("axisWidget");

std::string Dev_tools_manager::_name = "Dev_tools_manager";

template<>
IDev_tools_manager* oe::create_manager(
        Scene& scene, IScene_graph_manager& sceneGraphManager, IEntity_render_manager& entityRenderManager,
        IMaterial_manager& materialManager, IEntity_scripting_manager& entityScriptingManager)
{
  return new Dev_tools_manager(scene, sceneGraphManager, entityRenderManager, materialManager, entityScriptingManager);
}

void Dev_tools_manager::initialize() {
  _noLightProvider = [](const BoundingSphere&, std::vector<Entity*>&, uint32_t) {};
  _fpsCounter = std::make_unique<Fps_counter>();
  _animationControllers = _sceneGraphManager.getEntityFilter({Animation_controller_component::type()});
  _skinnedMeshEntities = _sceneGraphManager.getEntityFilter({Skinned_mesh_component::type()});
  _unlitMaterial = std::make_shared<Unlit_material>();
}

void Dev_tools_manager::shutdown() {
  _unlitMaterial.reset();
  _debugShapes.resize(0);
  _animationControllers = nullptr;
  _skinnedMeshEntities = nullptr;
  _fpsCounter.reset();
}

const std::string& Dev_tools_manager::name() const { return _name; }

void Dev_tools_manager::renderAxisWidgets() { addDebugAxisWidget(SSE::Matrix4::identity()); }

void Dev_tools_manager::renderSkeletons() {
  const auto white = oe::Colors::White;
  const auto red = oe::Colors::Red;

  std::list<Entity*> bones;
  for (const auto& skinnedMeshEntity : *_skinnedMeshEntities) {
    const auto component = skinnedMeshEntity->getFirstComponentOfType<Skinned_mesh_component>();
    assert(component);

    auto rootEntity = component->skeletonTransformRoot();
    if (!rootEntity)
      rootEntity = skinnedMeshEntity;

    for (const auto& child : rootEntity->children()) {
      bones.push_back(child.get());
    }

    while (!bones.empty()) {
      const auto bone = bones.front();
      bones.pop_front();

      for (const auto& child : bone->children()) {
        bones.push_back(child.get());
      }

      const auto parentPos = bone->parent()->worldPosition();
      const auto childPos = bone->worldPosition();

      auto targetDirection = childPos - parentPos;
      const auto height = SSE::length(targetDirection);

      if (height != 0.0f) {
        targetDirection /= height;

        SSE::Matrix3 rotation;
        if (!create_rotation_between_unit_vectors(rotation, math::up, targetDirection)) {
          rotation = SSE::Matrix3::rotationX(math::pi);
        }

        const auto worldPosition = parentPos + (rotation * SSE::Vector3{0, height * 0.5f, 0});
        const auto transform = SSE::Matrix4::translation(worldPosition);
        const auto scale = SSE::Matrix4::scale(SSE::Vector3(height));
        auto rotation4x4 = SSE::Matrix4::identity();
        rotation4x4.setUpper3x3(rotation);

        addDebugCone(transform * rotation4x4 * scale, 0.1f, 1.0f, red);
      }

      addDebugSphere(
          bone->worldTransform() * SSE::Matrix4::scale(SSE::Vector3(height)), 0.1f, white, 3);
    }
  }
}

void Dev_tools_manager::tick() {
  clearDebugShapes();
  if (_renderSkeletons)
    renderSkeletons();

  renderAxisWidgets();

  _fpsCounter->mark();
}

void Dev_tools_manager::createDeviceDependentResources() {}

void Dev_tools_manager::destroyDeviceDependentResources() {
  for (auto& debugShape : _debugShapes) {
    const auto& renderable = std::get<std::shared_ptr<Renderable>>(debugShape);
    renderable->rendererData.reset();
    renderable->materialContext.reset();
  }
}

void Dev_tools_manager::addDebugCone(
    const SSE::Matrix4& worldTransform,
    float diameter,
    float height,
    const Color& color) {
  auto hash = g_hashSeed_cone;
  hash_combine(hash, diameter);
  hash_combine(hash, height);

  auto renderable = getOrCreateRenderable(hash, [diameter, height]() {
    return Primitive_mesh_data_factory::createCone(diameter, height, 6);
  });

  _debugShapes.push_back({worldTransform, color, renderable});
}

void Dev_tools_manager::addDebugSphere(
    const SSE::Matrix4& worldTransform,
    float radius,
    const Color& color,
    size_t tessellation) {
  auto hash = g_hashSeed_sphere;
  hash_combine(hash, radius);
  hash_combine(hash, tessellation);

  auto renderable = getOrCreateRenderable(hash, [radius, tessellation]() {
    return Primitive_mesh_data_factory::createSphere(radius, tessellation);
  });

  _debugShapes.push_back({worldTransform, color, renderable});
}

void Dev_tools_manager::addDebugBoundingBox(
    const oe::BoundingOrientedBox& boundingOrientedBox,
    const Color& color) {
  auto worldTransform =
      SSE::Transform3(boundingOrientedBox.orientation, boundingOrientedBox.center);

  auto hash = g_hashSeed_boundingBox;
  hash_combine(hash, static_cast<float>(boundingOrientedBox.extents.getX()));
  hash_combine(hash, static_cast<float>(boundingOrientedBox.extents.getY()));
  hash_combine(hash, static_cast<float>(boundingOrientedBox.extents.getZ()));

  auto renderable = getOrCreateRenderable(hash, [&boundingOrientedBox]() {
    return Primitive_mesh_data_factory::createBox({boundingOrientedBox.extents.getX() * 2.0f,
                                                   boundingOrientedBox.extents.getY() * 2.0f,
                                                   boundingOrientedBox.extents.getZ() * 2.0f});
  });

  _debugShapes.push_back({SSE::Matrix4(worldTransform), color, renderable});
}

void Dev_tools_manager::addDebugFrustum(
    const BoundingFrustumRH& boundingFrustum,
    const Color& color) {
  // TODO: Cache these meshes? Will require not building the transform into the mesh itself.
  auto renderable = std::make_shared<Renderable>();
  renderable->meshData = Primitive_mesh_data_factory::createFrustumLines(boundingFrustum);
  renderable->material = _unlitMaterial;

  _debugShapes.push_back({SSE::Matrix4::identity(), color, renderable});
}

void Dev_tools_manager::addDebugAxisWidget(const SSE::Matrix4& worldTransform) {
  auto hash = g_hashSeed_axisWidget;
  auto renderable = getOrCreateRenderable(hash, []() {
    return Primitive_mesh_data_factory::createAxisWidgetLines();
  });

  _debugShapes.push_back({SSE::Matrix4(worldTransform), oe::Colors::Red, renderable});
}

void Dev_tools_manager::setGuiDebugText(const std::string& text) { _guiDebugText = text; }

void Dev_tools_manager::clearDebugShapes() { _debugShapes.clear(); }

void Dev_tools_manager::renderDebugShapes(const Camera_data& cameraData) {
  for (auto& debugShape : _debugShapes) {
    const auto& transform = std::get<SSE::Matrix4>(debugShape);
    const auto& color = std::get<Color>(debugShape);
    auto& renderable = std::get<std::shared_ptr<Renderable>>(debugShape);

    _unlitMaterial->setBaseColor(color);
    _entityRenderManager.renderRenderable(
        *renderable,
        transform,
        0.0f,
        cameraData,
        _noLightProvider,
        Render_pass_blend_mode::Opaque,
        true);
  }
}

void Dev_tools_manager::renderImGui() {
  // Display contents in a scrolling region
  ImGui::SetNextWindowSize(ImVec2(554, 300), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Renderer Features")) {
    auto featuresEnabled = _materialManager.rendererFeatureEnabled();

    const char* items[] = {debugDisplayModeToString(Debug_display_mode::None).c_str(),
                           debugDisplayModeToString(Debug_display_mode::Normals).c_str(),
                           debugDisplayModeToString(Debug_display_mode::World_positions).c_str(),
                           debugDisplayModeToString(Debug_display_mode::Lighting).c_str()};
    auto item_current = static_cast<int>(featuresEnabled.debugDisplayMode);
    if (ImGui::Checkbox("Skinning", &featuresEnabled.skinnedAnimation) ||
        ImGui::Checkbox("Morphing", &featuresEnabled.vertexMorph) ||
        ImGui::Checkbox("Shadowing", &featuresEnabled.shadowsEnabled) ||
        ImGui::Checkbox("Environment Irradiance", &featuresEnabled.irradianceMappingEnabled) ||
        ImGui::Checkbox("Shader Optimization", &featuresEnabled.enableShaderOptimization) ||
        ImGui::Combo("Debug Rendering", &item_current, items, IM_ARRAYSIZE(items))) {
      featuresEnabled.debugDisplayMode = static_cast<Debug_display_mode>(item_current);
      _materialManager.setRendererFeaturesEnabled(featuresEnabled);
    }
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(554, 675), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Animation")) {
    ImGui::Checkbox("Render Skeletons", &_renderSkeletons);

    for (const auto entity : *_animationControllers) {
      const auto animComponent = entity->getFirstComponentOfType<Animation_controller_component>();
      assert(animComponent);

      if (ImGui::CollapsingHeader(animComponent->getEntity().getName().c_str())) {
        if (!animComponent->activeAnimations.empty()) {
          if (ImGui::Button("Reset All Times")) {
            for (const auto& animEntry : animComponent->animations()) {
              auto activeAnimationPos = animComponent->activeAnimations.find(animEntry.first);
              if (activeAnimationPos == animComponent->activeAnimations.end())
                continue;

              for (size_t i = 0; i < activeAnimationPos->second.size(); ++i) {
                activeAnimationPos->second[i].currentTime = 0.0;
              }
            }
          }
        }

        for (const auto& animEntry : animComponent->animations()) {
          if (ImGui::TreeNode(animEntry.first.c_str())) {

            auto activeAnimationPos = animComponent->activeAnimations.find(animEntry.first);
            auto isActive = activeAnimationPos != animComponent->activeAnimations.end();

            if (ImGui::Checkbox("Loaded?", &isActive)) {
              if (isActive) {
                animComponent->activeAnimations[animEntry.first] = {};

                // Create the entry. States will be added in the next step.
                decltype(animComponent->activeAnimations)::value_type entry = {animEntry.first, {}};

                activeAnimationPos = animComponent->activeAnimations.insert(std::move(entry)).first;
              } else {
                animComponent->activeAnimations.erase(animEntry.first);
                activeAnimationPos = animComponent->activeAnimations.end();
              }
            }

            if (activeAnimationPos != animComponent->activeAnimations.end()) {

              if (!activeAnimationPos->second.empty()) {
                auto maxTime = 0.0f;
                for (const auto& channel : animEntry.second->channels) {
                  maxTime = std::max(maxTime, channel->keyframeTimes->back());
                }

                const auto firstTime = activeAnimationPos->second.front().currentTime;
                auto showResetButton = false;
                for (size_t i = 1; i < activeAnimationPos->second.size(); ++i) {
                  const auto& state = activeAnimationPos->second[i];
                  if (state.playing && (state.currentTime > firstTime + DBL_EPSILON ||
                                        state.currentTime < firstTime - DBL_EPSILON)) {
                    showResetButton = true;
                    break;
                  }
                }

                if (showResetButton) {
                  if (ImGui::Button("Zero Timeline")) {
                    for (size_t i = 0; i < activeAnimationPos->second.size(); ++i) {
                      activeAnimationPos->second[i].currentTime = 0.0;
                    }
                  }
                } else {
                  auto currentTime = static_cast<float>(firstTime);
                  if (ImGui::SliderFloat("Time", &currentTime, 0.0f, maxTime, "%.2f")) {
                    for (size_t i = 0; i < activeAnimationPos->second.size(); ++i) {
                      activeAnimationPos->second[i].currentTime = currentTime;
                    }
                  }
                }
              }

              if (ImGui::TreeNode("Channels")) {
                for (size_t i = 0; i < animEntry.second->channels.size(); ++i) {
                  const auto& channel = animEntry.second->channels[i];
                  const auto animationTypeStr = animationTypeToString(channel->animationType);

                  if (activeAnimationPos->second.size() <= i) {
                    activeAnimationPos->second.resize(animEntry.second->channels.size());
                  }

                  auto& state = activeAnimationPos->second[i];
                  ImGui::Checkbox(animationTypeStr.c_str(), &state.playing);
                }
                ImGui::TreePop();
              }
            }
            ImGui::TreePop();
          }
        }
      }
    }
  }
  ImGui::End();

  if (ImGui::Begin("Console")) {
    auto uiScale = ImGui::GetIO().FontGlobalScale;
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Logs");

    constexpr auto numMessageLevels = 3;
    ImGui::Columns(numMessageLevels, nullptr, false);
    static bool selected[numMessageLevels] = {false, true, true};
    ImGui::Selectable("DEBUG", &selected[0]);
    ImGui::NextColumn();
    ImGui::Selectable("INFO", &selected[1]);
    ImGui::NextColumn();
    ImGui::Selectable("WARNING", &selected[2]);
    ImGui::NextColumn();
    ImGui::Columns(1);

    const ImVec4 logColors[] = {
        {0.7f, 0.7f, 0.7f, 1},
        {1, 1, 1, 1},
        {1, 0, 0, 1},
    };

    if (ImGui::BeginChild(
            "scrolling",
            ImVec2(0, -25.0f * uiScale),
            false,
            ImGuiWindowFlags_HorizontalScrollbar)) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      const auto numLines = _vectorLog->messageCount();
      for (uint32_t i = 0; i < numLines; ++i) {
        const auto& message = _vectorLog->getMessageAt(i);
        auto level = 0;
        if (message.level >= g3::kInfoValue) {
          level = 1;
        }
        if (message.level >= g3::kWarningValue) {
          level = 2;
        }
        if (selected[level]) {
          const auto cStr = message.message.c_str();
          ImGui::TextColored(logColors[level], cStr);
        }
      }
      if (_scrollLogToBottom)
        ImGui::SetScrollHereY(1.0f);

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("commands", ImVec2(0, 20.0f * uiScale), false)) {
      if (ImGui::InputText(">", &_consoleInput)) {
        if (_consoleInput.size()) {
          if (_entityScriptingManager.commandSuggestions(
                  _consoleInput, _commandSuggestions)) {
            // TODO: Show suggestions
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Make some log!")) {
        _entityScriptingManager.execute(_consoleInput);
      }
      ImGui::SameLine(ImGui::GetWindowWidth() - 30 * uiScale);
      ImGui::Checkbox("Auto Scroll", &_scrollLogToBottom);
    }
    ImGui::EndChild();
  }
  ImGui::End();

  if (ImGui::Begin("Debug Statistics")) {
    ImGui::TextColored(ImVec4(1, 0, 1, 1), "Frame Time (s): %.4f", _fpsCounter->avgFrameTime());
    ImGui::TextColored(ImVec4(1, 0, 1, 1), "FPS: %.2f", _fpsCounter->avgFps());
    if (_guiDebugText.size()) {
      ImGui::Text(_guiDebugText.c_str());
    }
  }
  ImGui::End();
}

std::shared_ptr<Renderable> Dev_tools_manager::getOrCreateRenderable(
    size_t hash,
    std::function<std::shared_ptr<Mesh_data>()> factory) {
  const auto pos = _shapeMeshCache.find(hash);
  if (pos == _shapeMeshCache.end()) {
    const auto renderable = std::make_shared<Renderable>();
    renderable->meshData = factory();
    renderable->material = _unlitMaterial;
    _shapeMeshCache[hash] = renderable;
    return renderable;
  }

  return pos->second;
}
