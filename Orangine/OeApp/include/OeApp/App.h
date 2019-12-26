#pragma once

#include <string>

namespace oe {
class Scene;

struct App_start_settings {
  bool fullScreen = false;
  std::wstring title = L"";
};

class App {
 public:
  // A blocking call that will run the engine. Call this from your main() function.
  // When this returns, it is safe to exit the program
  int run(const App_start_settings& settings);

  virtual ~App() = default;

 protected:
  // You can implement this function in your application to load assets, etc.
  virtual void onSceneInitialized(Scene& scene) {};

  // Called just before the engine renders the next scene
  virtual void onSceneTick(Scene& scene) {};
};
} // namespace oe
