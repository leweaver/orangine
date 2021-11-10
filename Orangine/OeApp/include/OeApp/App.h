#pragma once

#include "OeCore/IDevice_resources.h"

#include "OeCore/WindowsDefines.h"

#include <memory>
#include <string>

#include <OeCore/Entity_repository.h>
#include <OeCore/IAnimation_manager.h>
#include <OeCore/IAsset_manager.h>
#include <OeCore/IBehavior_manager.h>
#include <OeCore/IDev_tools_manager.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IEntity_scripting_manager.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/IMaterial_manager.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/IShadowmap_manager.h>
#include <OeCore/ITexture_manager.h>
#include <OeCore/ITime_step_manager.h>
#include <OeCore/IUser_interface_manager.h>

namespace oe::app {
struct App_start_settings {
  bool fullScreen = false;
  std::wstring title = L"";
};

class App_impl;

class App {
 public:
  App() = default;
  virtual ~App() = default;
  App(const App& app) = delete;
  App& operator=(const App&) = delete;
  App(App&&) = delete;
  App& operator=(App&&) = delete;

  // A blocking call that will run the engine. Call this from your main() function.
  // When this returns, it is safe to exit the program
  int run(const App_start_settings& settings);

  // By default, returns a window that is 75% of the screen width and height (to a minimum window
  // size of 320x200px)
  void getDefaultWindowSize(int& w, int& h) const;

  int getScreenDpi() const;

  Entity_repository& getEntityRepository();
  ITime_step_manager& getTimeStepManager();
  IScene_graph_manager& getSceneGraphManager();
  IAsset_manager& getAssetManager();
  IUser_interface_manager& getUserInterfaceManager();
  ITexture_manager& getTextureManager();
  IBehavior_manager& getBehaviorManager();
  IMaterial_manager& getMaterialManager();
  ILighting_manager& getLightingManager();
  IShadowmap_manager& getShadowmapManager();
  IInput_manager& getInputManager();
  IAnimation_manager& getAnimationManager();
  IEntity_render_manager& getEntityRenderManager();
  IDev_tools_manager& getDevToolsManager();
  IRender_step_manager& getRenderStepManager();
  IEntity_scripting_manager& getEntityScriptingManager();

  // Internal only getter.
  App_impl* getAppImpl() const
  {
    return _impl;
  }

 protected:
  // Called after the scene & managers are created & configured, but have not yet been initialized.
  // You can implement this function in your application to call any pre-init functions on
  // scene manager classes.
  virtual void onSceneConfigured() {}

  // Called after the managers have been initialized.
  // You can implement this function in your application to load assets, etc.
  virtual void onSceneInitialized() {}

  // Called just before the engine ticks all of the inbuilt manager classes
  virtual void onScenePreTick() {}

  // Called after the managers are ticked, and just before the engine renders the next scene
  virtual void onScenePreRender() {}

  // Called when the application is about to shut down, before managers are destroyed.
  virtual void onSceneShutdown() {}

 private:
  // Platform specific internal implementation
  App_impl* _impl = nullptr;
};
}// namespace oe::app
