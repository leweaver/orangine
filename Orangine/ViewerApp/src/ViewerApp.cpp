#include "pch.h"

#include <ViewerAppConfig.h>

#include <OeApp/App.h>
#include <OeApp/SampleScene.h>
#include <OeCore/Color.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/Light_component.h>
#include <OeCore/PBR_material.h>
#include <OeCore/Scene.h>
#include <OeCore/WindowsDefines.h>
#include <OeScripting/Script_component.h>

#include <g3log/g3log.hpp>

#include <filesystem>

using namespace oe;

void CreateThreePointLights(Sample_scene& sampleScene) {
  const auto& lightRoot =
      sampleScene.getScene().manager<IScene_graph_manager>().instantiate("Light Root");
  lightRoot->setPosition({0, 0, 0});

  auto shadowLight1 = sampleScene.addDirectionalLight({0.0f, -1.0f, 0.0f}, {1, 1, 1, 1}, 2);
  shadowLight1->setParent(*lightRoot);
  shadowLight1->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(true);

  auto shadowLight2 =
      sampleScene.addDirectionalLight({-0.707f, -0.707f, -0.707f}, {1, 1, 0, 1}, 2.75);
  shadowLight2->setParent(*lightRoot);
  shadowLight2->getFirstComponentOfType<Directional_light_component>()->setShadowsEnabled(true);

  sampleScene.addDirectionalLight({-0.666f, -0.333f, 0.666f}, {1, 0, 1, 1}, 4.0)
      ->setParent(*lightRoot);

  // sampleScene.addAmbientLight({ 1, 1, 1 }, 0.2f)->setParent(*lightRoot);
}

void CreatePointPointLights(Sample_scene& sampleScene) {
  const auto& lightRoot =
      sampleScene.getScene().manager<IScene_graph_manager>().instantiate("Light Root");
  lightRoot->setPosition({0, 0, 0});

  sampleScene.addPointLight({10, 0, 10}, {1, 1, 1, 1}, 2 * 13)->setParent(*lightRoot);
  sampleScene.addPointLight({10, 5, -10}, {1, 0, 1, 1}, 2 * 20)->setParent(*lightRoot);
}

void CreateScripts(Scene& scene) {

  IScene_graph_manager& entityManager = scene.manager<IScene_graph_manager>();
  const auto& root1 = entityManager.instantiate("Script Container");

  auto& scriptComponent = root1->addComponent<Script_component>();
  scriptComponent.setScriptName("testmodule.TestComponent");
}

void CreateShadowTestScene(Sample_scene& sampleScene) {
  sampleScene.addCamera();
  sampleScene.addFloor();
  CreateThreePointLights(sampleScene);

  sampleScene.addTeapot({-2, 0, -2}, oe::Colors::Green, 1.0, 0.0f);
  sampleScene.addTeapot({2, 0, -2}, oe::Colors::Red, 1.0, 0.25f);
  sampleScene.addTeapot({-2, 0, 2}, oe::Colors::White, 1.0, 0.75f);
  sampleScene.addTeapot({2, 0, 2}, oe::Colors::Black, 1.0, 0.0f);
}

void CreateLightingTestScene(Sample_scene& sampleScene) {
  sampleScene.addCamera();
  sampleScene.addFloor();
}

class ViewerApp : public oe::App {
 public:
  void onSceneInitialized(Scene& scene) {
    _sampleScene.reset(new Sample_scene(scene, {utf8_decode(VIEWERAPP_THIRDPARTY_PATH)}));

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

    // CreateLights(scene);
    CreateShadowTestScene(*_sampleScene);
    //
    //
    // CreateLightingTestScene(*_sampleScene);
    CreateScripts(scene);

    // Load the skybox
    auto& assetManager = scene.manager<IAsset_manager>();
    auto& textureManager = scene.manager<ITexture_manager>();

    Environment_volume ev;
    ev.environmentIbl.skyboxTexture = textureManager.createTextureFromFile(
        assetManager.makeAbsoluteAssetPath(L"OeApp/textures/park-cubemap.dds"));
    ev.environmentIbl.iblBrdfTexture = textureManager.createTextureFromFile(
        assetManager.makeAbsoluteAssetPath(L"OeApp/textures/park-cubemapBrdf.dds"));
    ev.environmentIbl.iblDiffuseTexture = textureManager.createTextureFromFile(
        assetManager.makeAbsoluteAssetPath(L"OeApp/textures/park-cubemapDiffuseHDR.dds"));
    ev.environmentIbl.iblSpecularTexture = textureManager.createTextureFromFile(
        assetManager.makeAbsoluteAssetPath(L"OeApp/textures/park-cubemapSpecularHDR.dds"));

    scene.setEnvironmentVolume(ev);
  }

  void onSceneShutdown(Scene&) override { _sampleScene.reset(); }

 protected:
  std::unique_ptr<Sample_scene> _sampleScene;
};

// Entry point
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow) {
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