#pragma once

#include "OeCore/Scene.h"

namespace oe {
class Sample_scene {
 public:
  explicit Sample_scene(Scene& scene, const std::vector<std::wstring>& extraAssetPaths);

  void tick();

  Scene& getScene() const { return _scene; }
  std::shared_ptr<Entity> getRoot() const { return _root; }

  // Basics
  void addCamera();

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

 protected:
  std::string createLightEntityName(const char* lightType, const std::string& userName);

  void tickCamera();

  int _lightCount = 0;
  int _teapotCount = 0;

  std::vector<std::wstring> _extraAssetPaths;

  Scene& _scene;
  IScene_graph_manager& _entityManager;
  IInput_manager& _inputManager;

  std::shared_ptr<Entity> _root;
};
} // namespace oe
