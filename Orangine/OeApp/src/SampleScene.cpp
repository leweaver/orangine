#include "pch.h"

#include "OeApp/SampleScene.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Light_component.h"
#include "OeCore/PBR_material.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Test_component.h"

#include <filesystem>

using oe::Camera_component;
using oe::Directional_light_component;
using oe::Entity;
using oe::Sample_scene;

Sample_scene::Sample_scene(Scene& scene, const std::vector<std::wstring>& extraAssetPaths)
    : _extraAssetPaths(extraAssetPaths)
    , _scene(scene)
    , _entityManager(scene.manager<IScene_graph_manager>())
    , _inputManager(scene.manager<IInput_manager>()) {
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

void Sample_scene::addCamera() {
  auto cameraDollyAnchor = _entityManager.instantiate("CameraDollyAnchor");

  auto camera = _entityManager.instantiate("Camera", *cameraDollyAnchor);
  camera->setPosition({0.0f, 0.0f, 15});

  cameraDollyAnchor->computeWorldTransform();

  auto& component = camera->addComponent<Camera_component>();
  component.setFov(degrees_to_radians(60.0f));
  component.setFarPlane(20.0f);
  component.setNearPlane(0.5f);

  camera->lookAt({0, 0, 0}, math::up);

  _scene.setMainCamera(camera);

  _scene.manager<IEntity_scripting_manager>().loadSceneScript("camera.OrbitCamera");
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
  auto gltfPath = _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(
      utf8_decode("ViewerApp/data/meshes" + gltfPathSubfolder));
  LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);

  for (const auto& extraPath : _extraAssetPaths) {
    if (std::filesystem::exists(gltfPath)) {
      break;
    }
    std::wstringstream ss;
    ss << extraPath << utf8_decode(gltfPathSubfolder);

    gltfPath = ss.str();
    LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);
  }

  if (!std::filesystem::exists(gltfPath)) {
    OE_THROW(std::runtime_error("Could not find gltf file with name: " + gltfName));
  }

  _scene.loadEntities(gltfPath, *_root);

  return _root;
}