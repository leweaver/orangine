#pragma once

#include "OeCore/IDevice_resources.h"

#include "OeCore/WindowsDefines.h"

#include <memory>
#include <string>

namespace oe {
class Scene;
class Manager_base;

struct App_start_settings {
  bool fullScreen = false;
  std::wstring title = L"";
};

struct Scene_initializer {
  void addManager(std::unique_ptr<Manager_base> manager);
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

  // Internal only getter.
  App_impl* getAppImpl() const { return _impl; }

 protected:
  // Called after the scene & managers are created & configured, but have not yet been initialized.
  // You can implement this function in your application to call any pre-init functions on
  // scene manager classes.
  virtual void onSceneConfigured(Scene& scene) {}

  // Called after the managers have been initialized.
  // You can implement this function in your application to load assets, etc.
  virtual void onSceneInitialized(Scene& scene) {}

  // Called just before the engine ticks all of the inbuilt manager classes
  virtual void onScenePreTick(Scene& scene) {}

  // Called after the managers are ticked, and just before the engine renders the next scene
  virtual void onScenePreRender(Scene& scene) {}

  // Called when the application is about to shut down, before managers are destroyed.
  virtual void onSceneShutdown(Scene& scene) {}

private:

  // Platform specific internal implementation
  App_impl* _impl = nullptr;
};
} // namespace oe
