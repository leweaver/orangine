#include "pch.h"

#include "OeCore/Scene.h"

#include "OeCore/Camera_component.h"
#include "OeCore/Component.h"
#include "OeCore/Entity.h"
#include "OeCore/Entity_graph_loader_gltf.h"
#include "OeCore/IInput_manager.h"

#include "OeCore/Entity_repository.h"

#include "JsonConfigReader.h"
#include "OeCore/FileUtils.h"
#include "OeCore/IMaterial_manager.h"
#include "OeCore/Perf_timer.h"

#include <json.hpp>
#include <type_traits>

using namespace oe;

const auto CONFIG_FILE_NAME = L"config.json";

template<typename TBase, typename TTuple, int TIdx = 0>
constexpr void forEachOfType(TTuple& managers, const std::function<void(int, TBase*)>& fn)
{
  // Initialize only types that derive from TBase
  if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
    fn(TIdx, std::get<TIdx>(managers).get());

  // iterate the next manager
  if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>) forEachOfType<TBase, TTuple, TIdx + 1>(managers, fn);
}

template<typename TBase, typename TTuple, int TIdx = std::tuple_size_v<TTuple> - 1>
constexpr void forEachOfTypeReverse(TTuple& managers, const std::function<void(int, TBase*)>& fn)
{
  // Initialize only types that derive from TBase
  if constexpr (std::is_base_of_v<TBase, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
    fn(TIdx, std::get<TIdx>(managers).get());

  // iterate the next manager (in reverse order than initialized)
  if constexpr (TIdx > 0) forEachOfTypeReverse<TBase, TTuple, TIdx - 1>(managers, fn);
}

template<typename TTuple, int TIdx = 0>
constexpr void forEach_tick(TTuple& managers, std::array<double, std::tuple_size_v<TTuple>>& tickTimes)
{
  // Tick only types that derive from Manager_tickable
  if constexpr (std::is_base_of_v<Manager_tickable, std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>) {
    auto perfTimer = Perf_timer::start();
    std::get<TIdx>(managers)->tick();
    perfTimer.stop();
    tickTimes[TIdx] += perfTimer.elapsedSeconds();
  }

  // Recursively iterate to the next tuple index
  if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>) {
    forEach_tick<TTuple, TIdx + 1>(managers, tickTimes);
  }
}

template<typename TTuple, int TIdx = 0>
constexpr bool
forEach_processMessage(TTuple& managers, UINT message, WPARAM wParam, LPARAM lParam, bool handled = false)
{
  // Tick only types that derive from Manager_windowsMessageProcessor
  if constexpr (std::is_base_of_v<
                        Manager_windowsMessageProcessor,
                        std::remove_pointer_t<decltype(std::get<TIdx>(managers).get())>>)
  {
    if (!handled) {
      handled |= (std::get<TIdx>(managers)->processMessage(message, wParam, lParam));
    }
  }

  // Recursively iterate to the next tuple index
  if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>)
    return forEach_processMessage<TTuple, TIdx + 1>(managers, message, wParam, lParam, handled);

  return handled;
}

void Scene::configure()
{
  std::unique_ptr<JsonConfigReader> configReader;
  try {
    configReader = std::make_unique<JsonConfigReader>(CONFIG_FILE_NAME);
  }
  catch (std::exception& ex) {
    LOG(WARNING) << "Failed to read config file: " << utf8_encode(CONFIG_FILE_NAME) << "(" << ex.what() << ")";
    OE_THROW(std::runtime_error(std::string("Scene init failed reading config. ") + ex.what()));
  }

  try {
    forEachOfType<Manager_base>(_managers, [this, &configReader](auto, auto* manager) {
      if (manager == nullptr) return;

      try {
        manager->loadConfig(*configReader);
      }
      catch (std::exception& ex) {
        OE_THROW(std::runtime_error("Failed to configure " + manager->name() + ": " + ex.what()));
      }
    });
  }
  catch (std::exception& ex) {
    LOG(WARNING) << "Failed to configure managers: " << ex.what();
    OE_THROW(std::runtime_error(std::string("Scene configure failed. ") + ex.what()));
  }
}

void Scene::initialize()
{
  using namespace std;

  // Mesh loaders
  const auto meshLoader = addMeshLoader<Entity_graph_loader_gltf>();
  assert(meshLoader);

  std::fill(_tickTimes.begin(), _tickTimes.end(), 0.0);

  try {
    forEachOfType<Manager_base>(_managers, [this](auto, auto* manager) {
      if (manager == nullptr) {
        return;
      }

      try {
        manager->initialize();
      }
      catch (std::exception& ex) {
        OE_THROW(std::runtime_error("Failed to initialize " + manager->name() + ": " + ex.what()));
      }
      _initializedManagers.push_back(manager);
    });
  }
  catch (std::exception& ex) {
    LOG(WARNING) << "Failed to initialize managers: " << ex.what();
    OE_THROW(std::runtime_error(std::string("Scene init failed. ") + ex.what()));
  }
}

void Scene::loadEntities(const std::wstring& filename)
{
  return loadEntities(filename, nullptr);
}

void Scene::loadEntities(const std::wstring& filename, Entity& parentEntity)
{
  auto entity = _entityRepository->getEntityPtrById(parentEntity.getId());
  return loadEntities(filename, entity.get());
}

void Scene::loadEntities(const std::wstring& filename, Entity* parentEntity)
{
  // Get the file extension
  const auto dotPos = filename.find_last_of('.');
  if (dotPos == std::string::npos) {
    OE_THROW(std::runtime_error("Cannot load mesh; given file doesn't have an extension."));
  }

  const std::string extension = utf8_encode(filename.substr(dotPos + 1));
  const auto extPos = _entityGraphLoaders.find(extension);
  if (extPos == _entityGraphLoaders.end()) {
    OE_THROW(std::runtime_error("Cannot load mesh; no registered loader for extension: " + extension));
  }

  auto& sceneGraphManager = manager<IScene_graph_manager>();
  std::vector<std::shared_ptr<Entity>> newRootEntities = extPos->second->loadFile(
          filename, *_entityRepository, manager<IMaterial_manager>(), manager<ITexture_manager>(), dynamic_cast<IComponent_factory&>(sceneGraphManager), true);

  if (parentEntity) {
    for (const auto& entity : newRootEntities) {
      entity->setParent(*parentEntity);
    }
  }

  sceneGraphManager.handleEntitiesLoaded(newRootEntities);
}

void Scene::tick()
{
  _stepTimer.Tick([this]() {
    forEach_tick(_managers, _tickTimes);

    // Hack to reset timers after first frame which is typically quite heavy.
    if (_stepTimer.GetFrameCount() == 1) {
      std::fill(_tickTimes.begin(), _tickTimes.end(), 0.0);
    }
  });
}

void Scene::resumePlay()
{
  _stepTimer.ResetElapsedTime();
}

void Scene::shutdown()
{
  std::stringstream ss;
  const auto& tickTimes = _tickTimes;
  const double tickCount = getFrameCount();

  if (tickCount > 0) {
    const std::function<void(int, Manager_base*)> timesLog = [&](int idx, Manager_base* manager) {
      if (tickTimes[idx] > 0.0) {
        ss << "  " << manager->name() << ": " << (1000.0 * tickTimes[idx] / tickCount) << std::endl;
      }
    };
    forEachOfType(_managers, timesLog);
    LOG(INFO) << "Manager average tick times (ms): " << std::endl << ss.str();
  }

  std::for_each(
          _initializedManagers.rbegin(), _initializedManagers.rend(), [](const auto& manager) { manager->shutdown(); });
  _initializedManagers.clear();

  _managers = decltype(_managers)();
  _entityGraphLoaders.clear();
}

void Scene::setMainCamera(const std::shared_ptr<Entity>& cameraEntity)
{
  if (cameraEntity) {
    const auto cameras = cameraEntity->getComponentsOfType<Camera_component>();
    if (cameras.empty()) {
      OE_THROW(std::invalid_argument("Given entity must have exactly one CameraComponent"));
    }

    _mainCamera = cameraEntity;
  }
  else {
    _mainCamera = nullptr;
  }
}

void Scene_device_resource_aware::createWindowSizeDependentResources(HWND window, int width, int height)
{
  LOG(INFO) << "Creating window size dependent resources";
  forEachOfType<Manager_windowDependent>(_managers, [=](int, Manager_windowDependent* manager) {
    manager->createWindowSizeDependentResources(window, width, height);
  });
}

void Scene_device_resource_aware::destroyWindowSizeDependentResources()
{
  LOG(INFO) << "Destroying window size dependent resources";
  forEachOfType<Manager_windowDependent>(
          _managers, [](int, Manager_windowDependent* manager) { manager->destroyWindowSizeDependentResources(); });
}

void Scene_device_resource_aware::createDeviceDependentResources()
{
  LOG(INFO) << "Creating device dependent resources";
  forEachOfType<Manager_deviceDependent>(_managers, [this](int idx, Manager_deviceDependent* manager) {
    LOG(DEBUG) << "Creating device dependent resources for manager " << idx;
    manager->createDeviceDependentResources();
  });
}

void Scene_device_resource_aware::destroyDeviceDependentResources()
{
  LOG(INFO) << "Destroying device dependent resources";
  forEachOfType<Manager_deviceDependent>(
          _managers, [](int, Manager_deviceDependent* manager) { manager->destroyDeviceDependentResources(); });
}

bool Scene_device_resource_aware::processMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
  return forEach_processMessage(_managers, message, wParam, lParam);
}

Scene_device_resource_aware::Scene_device_resource_aware(IDevice_resources& deviceResources)
    : _deviceResources(deviceResources)
{
  using namespace std;

  // Repositories
  _entityRepository = make_shared<Entity_repository>(*this);

  // Managers with no dependencies
  createManager<ITime_step_manager>(static_cast<const StepTimer&>(_stepTimer));
  createManager<IScene_graph_manager>(_entityRepository);
  createManager<IAsset_manager>();
  createManager<IBehavior_manager>();

  // Only device dependencies
  createManager<IUser_interface_manager>(deviceResources);
  createManager<ITexture_manager>(deviceResources);

  createManager<IMaterial_manager>(manager<IAsset_manager>());
  createManager<ILighting_manager>(manager<ITexture_manager>());
  createManager<IShadowmap_manager>(manager<ITexture_manager>());
  createManager<IInput_manager>(manager<IUser_interface_manager>());

  createManager<IAnimation_manager>(manager<IScene_graph_manager>(), manager<ITime_step_manager>());
  createManager<IEntity_render_manager>(
          manager<ITexture_manager>(), manager<IMaterial_manager>(), manager<ILighting_manager>());

  createManager<IDev_tools_manager>(
          manager<IScene_graph_manager>(), manager<IEntity_render_manager>(), manager<IMaterial_manager>(),
          manager<IEntity_scripting_manager>());

  // Pulls everything together and draws pixels
  createManager<IRender_step_manager>(manager<IScene_graph_manager>(), manager<IDev_tools_manager>(),
                                      manager<ITexture_manager>(), manager<IShadowmap_manager>(),
                                      manager<IEntity_render_manager>(), manager<ILighting_manager>());

  // Provides access to the engine via python
  createManager<IEntity_scripting_manager>(
          manager<ITime_step_manager>(), manager<IScene_graph_manager>(), manager<IInput_manager>(),
          manager<IAsset_manager>(), manager<IEntity_render_manager>(), manager<IDev_tools_manager>());
}

void Scene_device_resource_aware::initialize()
{
  Scene::initialize();
}
