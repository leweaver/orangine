#pragma once

#include <string>

namespace DX {
class DeviceResources;
}

namespace oe {
class Scene;
class IConfigReader;

class Manager_tickable {
 public:
  virtual ~Manager_tickable() = default;
  virtual void tick() = 0;
};

class Manager_windowDependent {
 public:
  virtual ~Manager_windowDependent() = default;
  virtual void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window,
                                                  int width, int height) = 0;
  virtual void destroyWindowSizeDependentResources() = 0;
};

class Manager_deviceDependent {
 public:
  virtual ~Manager_deviceDependent() = default;
  virtual void createDeviceDependentResources(DX::DeviceResources& deviceResources) = 0;
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

  virtual void loadConfig(const IConfigReader&) {}
  virtual void initialize() = 0;
  virtual void shutdown() = 0;
  virtual const std::string& name() const = 0;

 protected:
  Scene& _scene;
};

// Specialize this factory method in the CPP of the concrete manager implementation
template <class TManager, class... TDependencies>
// ReSharper disable once CppFunctionIsNotImplemented
TManager* create_manager(Scene& scene, TDependencies&...);
} // namespace oe