#pragma once

#include <OeCore/Entity_graph_loader.h>
#include <OeCore/IAsset_manager.h>
#include <OeCore/IEntity_scripting_manager.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/IScene_graph_manager.h>

#include <OeCore/Color.h>

namespace oe {
class Sample_scene {
 public:
  Sample_scene(
          IRender_step_manager& renderStepManager, IScene_graph_manager& sceneGraphManager,
          IInput_manager& inputManager, IEntity_scripting_manager& entityScriptingManager,
          IAsset_manager& assetManager, std::vector<std::wstring> extraAssetPaths);

  void tick();

  std::shared_ptr<Entity> getRoot() const { return _root; }

  // Basics
  std::shared_ptr<Entity> addDefaultCamera();

  // Geometry
  std::shared_ptr<Entity> addFloor();
  std::shared_ptr<Entity>
  addTeapot(const SSE::Vector3& center, const Color& color, float metallic, float roughness);
  std::shared_ptr<Entity>
  addSphere(const SSE::Vector3& center, const Color& color, float metallic, float roughness);
  std::shared_ptr<Entity> addGltf(std::string gltfName);

  // Lights
  std::shared_ptr<Entity> addDirectionalLight(
      const SSE::Vector3& normal,
      const Color& color,
      float intensity,
      const std::string& name = "");
  std::shared_ptr<Entity> addPointLight(
      const SSE::Vector3& position,
      const Color& color,
      float intensity,
      const std::string& name = "");
  std::shared_ptr<Entity>
  addAmbientLight(const Color& color, float intensity, const std::string& name = "");

 private:
  std::string createLightEntityName(const char* lightType, const std::string& userName);

  void tickCamera();

  int _lightCount = 0;
  int _teapotCount = 0;

  std::vector<std::wstring> _extraAssetPaths;

  IRender_step_manager& _renderStepManager;
  IScene_graph_manager& _entityManager;
  IInput_manager& _inputManager;
  IEntity_scripting_manager& _entityScriptingManager;
  IAsset_manager& _assetManager;

  std::shared_ptr<Entity> _root;
};
} // namespace oe
