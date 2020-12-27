#pragma once

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

class App {
 public:
  // A blocking call that will run the engine. Call this from your main() function.
  // When this returns, it is safe to exit the program
  int run(const App_start_settings& settings);

  App() = default;
  virtual ~App() = default;
  App(const App&) = default;
  App& operator=(const App&) = default;
  App(App&&) = default;
  App& operator=(App&&) = default;

 protected:
  // Called after the scene & managers are created & configured, but have not yet been initialized.
  // You can implement this function in your application to call any pre-init functions on
  // scene manager classes.
  virtual void onSceneConfigured(Scene& scene) {}

  // Called after the managers have been initialized.
  // You can implement this function in your application to load assets, etc.
  virtual void onSceneInitialized(Scene& scene) {}

  // Called just before the engine ticks all of the inbuilt manager classes
  virtual void onSceneTick(Scene& scene) {}

  // Called after the managers are ticked, and just before the engine renders the next scene
  virtual void onScenePreRender(Scene& scene) {}

  // Called when the application is about to shut down, before managers are destroyed.
  virtual void onSceneShutdown(Scene& scene) {}
};
} // namespace oe
