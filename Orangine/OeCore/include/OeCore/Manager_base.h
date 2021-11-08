#pragma once

#include <string>

#include "OeCore/WindowsDefines.h"

namespace oe {
class Scene;
class IConfigReader;
class Behavior_manager;
namespace internal {
class Entity_render_manager;
}

class Manager_tickable {
 public:
  virtual ~Manager_tickable() = default;
  virtual void tick() = 0;
};

class Manager_windowDependent {
 public:
  virtual ~Manager_windowDependent() = default;
  virtual void createWindowSizeDependentResources(HWND window, int width, int height) = 0;
  virtual void destroyWindowSizeDependentResources() = 0;
};

class Manager_deviceDependent {
 public:
  virtual ~Manager_deviceDependent() = default;
  virtual void createDeviceDependentResources() = 0;
  virtual void destroyDeviceDependentResources() = 0;
};

class Manager_windowsMessageProcessor {
 public:
  virtual ~Manager_windowsMessageProcessor() = default;
  // Return true if the processor handled the message, and no further processing should occur.
  virtual bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0;
};


class Manager_base {
 public:
  explicit Manager_base(Scene& scene) : _scene(scene) {}
  virtual ~Manager_base() = default;
  Manager_base(const Manager_base&) = delete;
  Manager_base& operator=(const Manager_base&) = delete;
  Manager_base(Manager_base&&) = delete;
  Manager_base& operator=(Manager_base&&) = delete;

  virtual void loadConfig(const IConfigReader&) {}
  virtual void initialize() = 0;
  virtual void shutdown() = 0;
  virtual const std::string& name() const = 0;

 private:
  // TODO: these are hacks until we figure out a better way to expose the things from Scene.
  friend Behavior_manager;
  //friend internal::Entity_render_manager;
  Scene& _scene;
};

// Specialize this factory method in the CPP of the concrete manager implementation
template <class TManager, class... TDependencies>
// ReSharper disable once CppFunctionIsNotImplemented
TManager* create_manager(Scene& scene, TDependencies&...);
} // namespace oe