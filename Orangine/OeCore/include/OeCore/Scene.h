#pragma once

#include "Entity_graph_loader.h"
#include "StepTimer.h"

#include "IAnimation_manager.h"
#include "IAsset_manager.h"
#include "IBehavior_manager.h"
#include "IDev_tools_manager.h"
#include "IEntity_render_manager.h"
#include "IEntity_scripting_manager.h"
#include "IInput_manager.h"
#include "IRender_step_manager.h"
#include "IScene_graph_manager.h"
#include "IShadowmap_manager.h"
#include "IUser_interface_manager.h"

#include "IMaterial_manager.h"
#include "Manager_base.h"
#include "EngineUtils.h"

#include <map>
#include <memory>

namespace oe {
class Entity;
class Component;
class Texture;

class IEntity_repository;
class IMaterial_repository;
class IDevice_repository;

class Scene {
 public:
  Scene() = default;
  virtual ~Scene() = default;
  Scene(const Scene&) = default;
  Scene& operator=(const Scene&) = default;
  Scene(Scene&&) = default;
  Scene& operator=(Scene&&) = default;

  virtual void configure();
  virtual void initialize();
  void tick(DX::StepTimer const& timer);
  virtual void shutdown();

  template <typename TMgr> TMgr& manager() const;

  /** Total time since game start, in seconds. */
  double elapsedTime() const { return _elapsedTime; }

  /** Time since last frame, in seconds. */
  double deltaTime() const { return _deltaTime; }

  /*
   * Functions that can be used to send events to various managers
   */
  void onComponentAdded(Entity& entity, Component& component) const;
  void onComponentRemoved(Entity& entity, Component& component) const;
  void onEntityAdded(Entity& entity) const;
  void onEntityRemoved(Entity& entity) const;

  /*
   * Add things to the scene, from a file
   */
  void loadEntities(const std::wstring& filename);
  void loadEntities(const std::wstring& filename, Entity& parentEntity);

  template <typename TLoader> std::shared_ptr<TLoader> addMeshLoader()
  {
    const std::shared_ptr<TLoader> ml = std::make_shared<TLoader>();
    std::vector<std::string> extensions;
    ml->getSupportedFileExtensions(extensions);
    for (const auto& extension : extensions) {
      if (_entityGraphLoaders.find(extension) != _entityGraphLoaders.end()) {
        OE_THROW(std::runtime_error(
            "Failed to register entity graph loader, file extension is already registered: " +
            extension));
      }
      _entityGraphLoaders[extension] = ml;
    }
    return ml;
  }

  std::shared_ptr<Entity> mainCamera() const { return _mainCamera; }
  void setMainCamera(const std::shared_ptr<Entity>& cameraEntity);

  std::shared_ptr<Texture> skyboxTexture() const { return _skyBoxTexture; }
  void setSkyboxTexture(std::shared_ptr<Texture> skyBoxTexture) { _skyBoxTexture = skyBoxTexture; }

 protected:
  // Repositories
  std::shared_ptr<IEntity_repository> _entityRepository;
  std::shared_ptr<IMaterial_repository> _materialRepository;
  std::shared_ptr<IDevice_repository> _deviceRepository;

  // Managers
  using Manager_tuple = std::tuple<
      // Managers. IUser_interface_manager should always be first, so it can handle windows messages
      // first.
      std::shared_ptr<IUser_interface_manager>,   //
      std::shared_ptr<IAsset_manager>,            //
      std::shared_ptr<IScene_graph_manager>,      //
      std::shared_ptr<IDev_tools_manager>,        //
      std::shared_ptr<IEntity_render_manager>,    //
      std::shared_ptr<IRender_step_manager>,      //
      std::shared_ptr<IShadowmap_manager>,        //
      std::shared_ptr<IEntity_scripting_manager>, //
      std::shared_ptr<IInput_manager>,            //
      std::shared_ptr<IBehavior_manager>,  //
      std::shared_ptr<IAnimation_manager>,        //
      std::shared_ptr<IMaterial_manager>>         //
      ;

  Manager_tuple _managers;
  std::vector<Manager_base*> _initializedManagers;

  std::shared_ptr<Texture> _skyBoxTexture;

 private:
  void loadEntities(const std::wstring& filename, Entity* parentEntity);

  std::map<std::string, std::shared_ptr<Entity_graph_loader>> _entityGraphLoaders = {};

  std::shared_ptr<Entity> _mainCamera = nullptr;

  // Values as of the last call to Tick.
  double _deltaTime = 0;
  double _elapsedTime = 0;

  std::array<double, std::tuple_size_v<Manager_tuple>> _tickTimes = {};
  uint32_t _tickCount = 0;
};

template <typename TMgr> TMgr& Scene::manager() const
{
  return *std::get<std::shared_ptr<TMgr>>(_managers);
}

class Scene_device_resource_aware : public Scene {
 public:
  explicit Scene_device_resource_aware(DX::DeviceResources& deviceResources);

  void initialize() override;

  void createWindowSizeDependentResources(HWND window, int width, int height);
  void destroyWindowSizeDependentResources();
  void createDeviceDependentResources();
  void destroyDeviceDependentResources();
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) const;

 private:
  template <class TManager, class... TDependencies>
  void createManager(TDependencies&... dependencies)
  {
    std::get<std::shared_ptr<TManager>>(_managers) = std::unique_ptr<TManager>(
        create_manager<TManager, TDependencies...>(*this, dependencies...));
  }
  DX::DeviceResources& _deviceResources;
};
} // namespace oe
