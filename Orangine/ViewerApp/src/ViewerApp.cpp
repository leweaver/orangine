#include "pch.h"

#include "ViewerAppConfig.h"

#include <OeCore/WindowsDefines.h>
#include <OeCore/Camera_component.h>
#include <OeCore/Collision.h>
#include <OeCore/Color.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/Light_component.h>
#include <OeCore/Math_constants.h>
#include <OeCore/PBR_material.h>
#include <OeCore/Renderable_component.h>
#include <OeCore/Scene.h>
#include <OeCore/Test_component.h>
#include <OeScripting/Script_component.h>
#include <OeApp/App.h>

#include <g3log/g3log.hpp>

#include <filesystem>

using namespace oe;

std::shared_ptr<Entity> LoadGLTFToEntity(Scene& scene, std::string gltfName,
                                         std::shared_ptr<Entity> root)
{
  const auto gltfPathSubfolder = "/" + gltfName + "/glTF/" + gltfName + ".gltf";
  auto gltfPath = scene.manager<IAsset_manager>().makeAbsoluteAssetPath(
      utf8_decode("ViewerApp/data/meshes" + gltfPathSubfolder));
  LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);

  if (!std::filesystem::exists(gltfPath)) {
    std::wstringstream ss;
    ss << VIEWERAPP_THIRDPARTY_PATH << utf8_decode("/glTF-Sample-Models/2.0")
       << utf8_decode(gltfPathSubfolder);
    gltfPath = ss.str();

    LOG(DEBUG) << "Looking for gltf at: " << utf8_encode(gltfPath);
    if (!std::filesystem::exists(gltfPath)) {
      throw std::runtime_error("Could not find gltf file with name: " + gltfName);
    }
  }

  scene.loadEntities(gltfPath, *root);

  return root;
}

void AddCubeToEntity(std::shared_ptr<Entity> entity, SSE::Vector3 animationSpeed,
                     SSE::Vector3 localScale, SSE::Vector3 localPosition)
{
  entity->addComponent<Test_component>().setSpeed(animationSpeed);

  entity->setScale(localScale);
  entity->setPosition(localPosition);

  auto child = LoadGLTFToEntity(entity->scene(), "Cube", entity);
}

void CreateSceneCubeSatellite(Scene& scene)
{
  auto& entityManager = scene.manager<IScene_graph_manager>();
  const auto root1 = entityManager.instantiate("Root 1");
  // root1->AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.1250f, 0.06f, 0.0f));

  const auto child1 = entityManager.instantiate("Child 1", *root1);
  const auto child2 = entityManager.instantiate("Child 2", *root1);
  const auto child3 = entityManager.instantiate("Child 3", *root1);

  AddCubeToEntity(child1, {0.5f, 0.0f, 0.0f}, {2, 1, 1}, {4, 0, 0});
  AddCubeToEntity(child2, {0.0f, 0.5f, 0.0f}, {1, 2, 1}, {0, 0, 0});
  AddCubeToEntity(child3, {0.0f, 0.0f, 0.5f}, {1, 1, 2}, {-4, 0, 0});
}

std::shared_ptr<Entity> LoadGLTF(Scene& scene, std::string gltfName, bool animate)
{
  auto& entityManager = scene.manager<IScene_graph_manager>();
  auto root = entityManager.instantiate("Root");

  if (animate)
    root->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});

  return LoadGLTFToEntity(scene, gltfName, root);
}

void CreateCamera(Scene& scene, bool animate)
{
  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();

  auto cameraDollyAnchor = entityManager.instantiate("CameraDollyAnchor");
  if (animate)
    cameraDollyAnchor->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});

  auto camera = entityManager.instantiate("Camera", *cameraDollyAnchor);
  camera->setPosition({0.0f, 0.0f, 15});

  cameraDollyAnchor->computeWorldTransform();

  auto& component = camera->addComponent<Camera_component>();
  component.setFov(degrees_to_radians(60.0f));
  component.setFarPlane(20.0f);
  component.setNearPlane(0.5f);

  camera->lookAt({0, 0, 0}, math::up);

  scene.setMainCamera(camera);
}

void CreateLights(Scene& scene)
{
  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();
  int lightCount = 0;
  auto createDirLight = [&entityManager, &lightCount](const SSE::Vector3& normal,
                                                      const Color& color, float intensity) {
    auto lightEntity =
        entityManager.instantiate("Directional Light " + std::to_string(++lightCount));
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
  };
  auto createPointLight = [&entityManager, &lightCount](const SSE::Vector3& position,
                                                        const Color& color, float intensity) {
    auto lightEntity = entityManager.instantiate("Point Light " + std::to_string(++lightCount));
    auto& component = lightEntity->addComponent<Point_light_component>();
    component.setColor(color);
    component.setIntensity(intensity);
    lightEntity->setPosition(position);
    return lightEntity;
  };
  auto createAmbientLight = [&entityManager, &lightCount](const Color& color, float intensity) {
    auto lightEntity = entityManager.instantiate("Ambient Light " + std::to_string(++lightCount));
    auto& component = lightEntity->addComponent<Ambient_light_component>();
    component.setColor(color);
    component.setIntensity(intensity);
    return lightEntity;
  };

  const auto& lightRoot = entityManager.instantiate("Light Root");
  lightRoot->setPosition({0, 0, 0});

  if (true) {
    auto shadowLight1 = createDirLight({0.0f, -1.0f, 0.0f}, {1, 1, 1, 1}, 2);
    shadowLight1->setParent(*lightRoot);
    shadowLight1->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(true);

    auto shadowLight2 = createDirLight({-0.707f, -0.707f, -0.707f}, {1, 1, 0, 1}, 2.75);
    shadowLight2->setParent(*lightRoot);
    shadowLight2->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(true);

    createDirLight({-0.666f, -0.333f, 0.666f}, {1, 0, 1, 1}, 4.0)->setParent(*lightRoot);
  }
  else {
    createPointLight({10, 0, 10}, {1, 1, 1, 1}, 2 * 13)->setParent(*lightRoot);
    createPointLight({10, 5, -10}, {1, 0, 1, 1}, 2 * 20)->setParent(*lightRoot);
  }

  // createAmbientLight({ 1, 1, 1 }, 0.2f)->setParent(*lightRoot);
}

void CreateSceneLeverArm(Scene& scene)
{
  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();
  const auto root1 = entityManager.instantiate("Root 1");
  root1->setPosition({5, 0, 5});

  const auto child1 = entityManager.instantiate("Child 1", *root1);
  const auto child2 = entityManager.instantiate("Child 2", *child1);
  const auto child3 = entityManager.instantiate("Child 3", *child2);

  AddCubeToEntity(child1, {0, 0, 0.1f}, {0.5f, 0.5f, 1.0f}, {0, 0, 0});
  AddCubeToEntity(child2, {0, 0.25, 0}, {1.0f, 2.0f, 0.5f}, {0, 0, 2});
  AddCubeToEntity(child3, {0.5f, 0, 0}, {2.0f, 0.50f, 1.0f}, {0, 2, 0});
}

void CreateScripts(Scene& scene)
{

  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();
  const auto& root1 = entityManager.instantiate("Script Container");

  auto& scriptComponent = root1->addComponent<Script_component>();
  scriptComponent.setScriptName("testmodule.TestComponent");
}

void CreateShadowTestScene(Scene& scene)
{
  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();
  const auto& root1 = entityManager.instantiate("Primitives");
  int teapotCount = 0;

  auto createTeapot = [&entityManager, &root1, &teapotCount](const SSE::Vector3& center,
                                                             const Color& color, float metallic,
                                                             float roughness) {
    const auto& child1 =
        entityManager.instantiate("Teapot " + std::to_string(++teapotCount), *root1);
    child1->setPosition(center);

    std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
    material->setBaseColor(color);
    material->setMetallicFactor(metallic);
    material->setRoughnessFactor(roughness);

    auto& renderable = child1->addComponent<Renderable_component>();
    renderable.setMaterial(std::unique_ptr<Material>(material.release()));
    renderable.setWireframe(false);
    renderable.setCastShadow(true);

    const auto meshData = Primitive_mesh_data_factory::createTeapot();
    child1->addComponent<Mesh_data_component>().setMeshData(meshData);
    child1->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 1.0f));
    child1->addComponent<Test_component>().setSpeed({0.0f, 0.1f, 0.0f});
  };
  auto createSphere = [&entityManager, &root1](const SSE::Vector3& center, const Color& color,
                                               float metallic, float roughness) {
    const auto& child1 = entityManager.instantiate("Primitive Child 1", *root1);
    child1->setPosition(center);

    std::unique_ptr<PBR_material> material = std::make_unique<PBR_material>();
    material->setBaseColor(color);
    material->setMetallicFactor(metallic);
    material->setRoughnessFactor(roughness);

    auto& renderable = child1->addComponent<Renderable_component>();
    renderable.setMaterial(std::unique_ptr<Material>(material.release()));
    renderable.setWireframe(false);
    renderable.setCastShadow(false);

    const auto meshData = Primitive_mesh_data_factory::createSphere();
    child1->addComponent<Mesh_data_component>().setMeshData(meshData);
    child1->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 1.0f));
  };

  int created = 0;
  // for (float metallic = 0.0f; metallic <= 1.0f; metallic += 0.2f) {
  //    for (float roughness = 0.0f; roughness <= 1.0f; roughness += 0.2f) {
  //        createSphere({ metallic * 10.0f - 5.0f,  roughness * 10.0f - 5.0f, 0 },
  //        Color(Colors::White), 1.0f - metallic, 1.0f - roughness); created++;
  //    }
  //}

  if (true) {
    // Create the floor
    const auto& child2 = entityManager.instantiate("Primitive Child 2", *root1);
    auto material = std::make_unique<PBR_material>();
    material->setBaseColor(Color(0.7f, 0.7f, 0.7f, 1.0f));

    auto& renderable = child2->addComponent<Renderable_component>();
    renderable.setMaterial(std::unique_ptr<Material>(material.release()));
    renderable.setCastShadow(false);

    const auto meshData = Primitive_mesh_data_factory::createQuad(20, 20);
    child2->addComponent<Mesh_data_component>().setMeshData(meshData);

    child2->setRotation(SSE::Quat::rotationX(math::pi * -0.5f));
    child2->setPosition({0.0f, -1.5f, 0.0f});
    child2->setBoundSphere(oe::BoundingSphere(SSE::Vector3(0), 10.0f));
  }

  createTeapot({-2, 0, -2}, oe::Colors::Green, 1.0, 0.0f);
  createTeapot({2, 0, -2}, oe::Colors::Red, 1.0, 0.25f);
  createTeapot({-2, 0, 2}, oe::Colors::White, 1.0, 0.75f);
  createTeapot({2, 0, 2}, oe::Colors::Black, 1.0, 0.0f);
}

class ViewerApp : public oe::App {
 protected:
  void onSceneInitialized(Scene& scene)
  {
    // CreateSceneCubeSatellite();
    // CreateSceneLeverArm();
    // LoadGLTF("Avocado", true)->setScale({ 120, 120, 120 });

    // LoadGLTF("NormalTangentTest", false)->setScale({ 7, 7, 7 });
    // LoadGLTF("AlphaBlendModeTest", false)->setScale({3, 3, 3});
    // LoadGLTF("FlightHelmet", false)->setScale({ 7, 7, 7 });
    // LoadGLTF("WaterBottle", true)->setScale({ 40, 40, 40 });
    // LoadGLTF("InterpolationTest", false);
    // LoadGLTF("MorphPrimitivesTest", false)->setScale({2, 2, 2});
    // LoadGLTF("AnimatedMorphCube", false)->setPosition({ 0, -3.0f, 0 });
    // LoadGLTF("Alien", false)->setScale({ 10.01f, 10.01f, 10.01f });
    // LoadGLTF("MorphCube2", false);
    // LoadGLTF("RiggedSimple", false);
    // LoadGLTF("RiggedFigure", false);
    // LoadGLTF("CesiumMan", false)->setPosition({ 0, 0, 1.0f });

    // LoadGLTF("VC", false)->setPosition({ 0, 0, 0 });
    // LoadGLTF("WaterBottle", true)->setScale({ 40, 40, 40 });

    // LoadGLTF("MetalRoughSpheres", false);

    CreateCamera(scene, false);
    CreateLights(scene);
    CreateShadowTestScene(scene);
    CreateScripts(scene);

    // Load the skybox
    auto skyBoxTexture =
        std::make_shared<File_texture>(scene.manager<IAsset_manager>().makeAbsoluteAssetPath(
            L"OeApp/textures/park-cubemap.dds"));
    scene.setSkyboxTexture(skyBoxTexture);
  }
};

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  nlohmann::json::value_type configJson;

  auto startSettings = App_start_settings();
  startSettings.fullScreen = nCmdShow == SW_SHOWMAXIMIZED;

  auto app = ViewerApp();
  int retVal = app.run(startSettings);
  return retVal;
}