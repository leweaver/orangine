#include "OeApp/SampleScene.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Light_component.h"
#include "OeCore/PBR_material.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Test_component.h"

#include <OeCore/Entity_graph_loader_gltf.h>
#include <OeCore/Primitive_mesh_data_factory.h>
#include <filesystem>

using oe::Camera_component;
using oe::Directional_light_component;
using oe::Entity;
using oe::Sample_scene;

Sample_scene::Sample_scene(
        IRender_step_manager& renderStepManager, IScene_graph_manager& sceneGraphManager, IInput_manager& inputManager,
        IEntity_scripting_manager& entityScriptingManager, IAsset_manager& assetManager, std::vector<std::string> extraAssetPaths)
    : _extraAssetPaths(std::move(extraAssetPaths))
    , _renderStepManager(renderStepManager)
    , _entityManager(sceneGraphManager)
    , _inputManager(inputManager)
    , _entityScriptingManager(entityScriptingManager)
    , _assetManager(assetManager)
{
  _root = _entityManager.instantiate("Sample Scene Root");
}

void Sample_scene::tick() { tickCamera(); }

void Sample_scene::tickCamera() {
  const auto mouseState = _inputManager.getMouseState().lock();
  if (mouseState == nullptr) {
    return;
  }

  if (mouseState->left == IInput_manager::Mouse_state::Button_state::Held) {
    
  }
}

std::shared_ptr<Entity> Sample_scene::addFloor() {
  const auto& floor = _entityManager.instantiate("Floor", *_root);
  auto material = std::make_unique<PBR_material>();
  material->setBaseColor(Color(0.7f, 0.7f, 0.7f, 1.0f));

  auto& renderable = floor->addComponent<Renderable_component>();
  renderable.setMaterial(std::unique_ptr<Material>(material.release()));
  renderable.setCastShadow(false);

  const auto meshData = Primitive_mesh_data_factory::createQuad(20, 20);
  floor->addComponent<Mesh_data_component>().setMeshData(meshData);

  floor->setRotation(SSE::Quat::rotationX(math::pi * -0.5f));
  floor->setPosition({0.0f, -1.5f, 0.0f});
  floor->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 10.0f));

  return floor;
}

std::shared_ptr<Entity> Sample_scene::addTeapot(
    const SSE::Vector3& center,
    const Color& color,
    float metallic,
    float roughness) {
  _teapotCount++;

  const auto& teapot = _entityManager.instantiate("Teapot " + std::to_string(_teapotCount), *_root);
  teapot->setPosition(center);

  auto material = std::make_unique<PBR_material>();
  material->setBaseColor(color);
  material->setMetallicFactor(metallic);
  material->setRoughnessFactor(roughness);

  auto& renderable = teapot->addComponent<Renderable_component>();
  renderable.setMaterial(std::unique_ptr<Material>(material.release()));
  renderable.setWireframe(false);
  renderable.setCastShadow(true);

  const auto meshData = Primitive_mesh_data_factory::createTeapot();
  teapot->addComponent<Mesh_data_component>().setMeshData(meshData);
  teapot->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 1.0f));
  teapot->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});

  return teapot;
}

std::shared_ptr<Entity> Sample_scene::addSphere(
    const SSE::Vector3& center,
    const Color& color,
    float metallic,
    float roughness) {
  const auto& sphere = _entityManager.instantiate("Primitive Child 1", *_root);
  sphere->setPosition(center);

  std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
  material->setBaseColor(color);
  material->setMetallicFactor(metallic);
  material->setRoughnessFactor(roughness);

  auto& renderable = sphere->addComponent<Renderable_component>();
  renderable.setMaterial(std::unique_ptr<Material>(material.release()));
  renderable.setWireframe(false);
  renderable.setCastShadow(false);

  const auto meshData = Primitive_mesh_data_factory::createSphere();
  sphere->addComponent<Mesh_data_component>().setMeshData(meshData);
  sphere->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 1.0f));

  return sphere;
}

std::shared_ptr<Entity> Sample_scene::addDefaultCamera() {
  auto cameraDollyAnchor = _entityManager.instantiate("CameraDollyAnchor");

  auto camera = _entityManager.instantiate("Camera", *cameraDollyAnchor);
  camera->setPosition({0.0f, 0.0f, 15});

  cameraDollyAnchor->computeWorldTransform();

  auto& component = camera->addComponent<Camera_component>();
  component.setFov(degrees_to_radians(60.0f));
  component.setFarPlane(20.0f);
  component.setNearPlane(0.5f);

  camera->lookAt({0, 0, 0}, math::up);

  _renderStepManager.setCameraEntity(camera);
  _entityScriptingManager.loadSceneScript("camera_controller.FirstPersonController");

  return camera;
}

std::shared_ptr<Entity> Sample_scene::addDirectionalLight(
    const SSE::Vector3& normal,
    const Color& color,
    float intensity,
    const std::string& name) {
  auto lightEntity = _entityManager.instantiate(createLightEntityName("Directional", name));
  auto& component = lightEntity->addComponent<Directional_light_component>();
  component.setColor(color);
  component.setIntensity(intensity);

  // if normal is NOT forward vector
  if (SSE::lengthSqr(normal - math::forward) != 0.0f) {
    SSE::Vector3 axis;
    // if normal is backward vector
    if (SSE::lengthSqr(normal - math::backward) == 0.0f)
      axis = math::up;
    else {
      axis = SSE::cross(math::forward, normal);
      axis = SSE::normalize(axis);
    }

    assert(SSE::lengthSqr(normal) != 0);
    float angle = acos(SSE::dot(math::forward, normal) / SSE::length(normal));
    lightEntity->setRotation(SSE::Quat::rotation(angle, axis));
  }
  return lightEntity;
}

std::shared_ptr<Entity> Sample_scene::addPointLight(
    const SSE::Vector3& position,
    const Color& color,
    float intensity,
    const std::string& name) {
  auto lightEntity = _entityManager.instantiate(createLightEntityName("Point", name));
  auto& component = lightEntity->addComponent<Point_light_component>();
  component.setColor(color);
  component.setIntensity(intensity);
  lightEntity->setPosition(position);
  return lightEntity;
}

std::shared_ptr<Entity>
Sample_scene::addAmbientLight(const Color& color, float intensity, const std::string& name) {
  auto lightEntity = _entityManager.instantiate(createLightEntityName("Ambient", name));
  auto& component = lightEntity->addComponent<Ambient_light_component>();
  component.setColor(color);
  component.setIntensity(intensity);
  return lightEntity;
}

std::string Sample_scene::createLightEntityName(
    const char* lightType,
    const std::string& userName) {
  _lightCount++;

  std::stringstream ss;
  ss << lightType << " " << _lightCount;
  if (!userName.empty()) {
    ss << "(" << userName << ")";
  }

  return ss.str();
}

std::shared_ptr<Entity> Sample_scene::addGltf(std::string gltfName) {
  const auto gltfPathSubfolder = "/" + gltfName + "/glTF/" + gltfName + ".gltf";
  auto gltfPath = _assetManager.makeAbsoluteAssetPath(
      "ViewerApp/data/meshes" + gltfPathSubfolder);
  LOG(DEBUG) << "Looking for gltf at: " << gltfPath;

  for (const auto& extraPath : _extraAssetPaths) {
    if (std::filesystem::exists(gltfPath)) {
      break;
    }
    std::stringstream ss;
    ss << extraPath << gltfPathSubfolder;

    gltfPath = ss.str();
    LOG(DEBUG) << "Looking for gltf at: " << gltfPath;
  }

  if (!std::filesystem::exists(gltfPath)) {
    OE_THROW(std::runtime_error("Could not find gltf file with name: " + gltfName));
  }

  _entityManager.loadFile(gltfPath, _root.get());

  return _root;
}