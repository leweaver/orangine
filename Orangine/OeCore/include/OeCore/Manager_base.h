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
  Manager_base() = default;
  virtual ~Manager_base() = default;
  Manager_base(const Manager_base&) = delete;
  Manager_base& operator=(const Manager_base&) = delete;
  Manager_base(Manager_base&&) = delete;
  Manager_base& operator=(Manager_base&&) = delete;

  virtual void loadConfig(const IConfigReader&) {}
  virtual void initialize() = 0;
  virtual void shutdown() = 0;
  virtual const std::string& name() const = 0;
};

struct Manager_interfaces {
  Manager_interfaces()
      : asBase(nullptr)
      , asTickable(nullptr)
      , asDeviceDependent(nullptr)
      , asWindowDependent(nullptr)
      , asWindowsMessageProcessor(nullptr)
  {}
  Manager_interfaces(
          Manager_base* asBase, Manager_tickable* asTickable, Manager_deviceDependent* asDeviceDependent,
          Manager_windowDependent* asWindowDependent, Manager_windowsMessageProcessor* asWindowsMessageProcessor)
      : asBase(asBase)
      , asTickable(asTickable)
      , asDeviceDependent(asDeviceDependent)
      , asWindowDependent(asWindowDependent)
      , asWindowsMessageProcessor(asWindowsMessageProcessor)
  {}

  template<class TClass> static Manager_interfaces create(TClass* instance)
  {
    Manager_tickable* asTickable = nullptr;
    Manager_deviceDependent* asDeviceDependent = nullptr;
    Manager_windowDependent* asWindowDependent = nullptr;
    Manager_windowsMessageProcessor* asWindowsMessageProcessor = nullptr;

    static_assert(std::is_convertible_v<TClass*, Manager_base*>);
    if constexpr (std::is_convertible_v<TClass*, Manager_tickable*>) {
      asTickable = instance;
    }
    if constexpr (std::is_convertible_v<TClass*, Manager_deviceDependent*>) {
      asDeviceDependent = instance;
    }
    if constexpr (std::is_convertible_v<TClass*, Manager_windowDependent*>) {
      asWindowDependent = instance;
    }
    if constexpr (std::is_convertible_v<TClass*, Manager_windowsMessageProcessor*>) {
      asWindowsMessageProcessor = instance;
    }

    return {instance, asTickable, asDeviceDependent, asWindowDependent, asWindowsMessageProcessor};
  }

  Manager_base* asBase;
  Manager_tickable* asTickable;
  Manager_deviceDependent* asDeviceDependent;
  Manager_windowDependent* asWindowDependent;
  Manager_windowsMessageProcessor* asWindowsMessageProcessor;
};

template<class TManager> struct Manager_instance {
  Manager_instance()
      : interfaces({})
      , instance(nullptr)
  {}
  Manager_instance(Manager_interfaces interfaces, std::unique_ptr<TManager> instance)
      : interfaces(interfaces)
      , instance(std::move(instance))
  {}

  template<class TConcreteManager>
  explicit Manager_instance(std::unique_ptr<TConcreteManager> concreteInstance)
      : interfaces(Manager_interfaces::create(concreteInstance.get()))
      , instance(std::move(concreteInstance))
  {}
  Manager_interfaces interfaces;
  std::unique_ptr<TManager> instance;
};

// Specialize this factory method in the CPP of the concrete manager implementation
template<class TManager, class... TDependencies>
// ReSharper disable once CppFunctionIsNotImplemented
void create_manager(Manager_instance<TManager>&, TDependencies&...);

template<class TManager, class... TDependencies>
// ReSharper disable once CppFunctionIsNotImplemented
Manager_instance<TManager> create_manager_instance(TDependencies&... args) {
  Manager_instance<TManager> instance;
  create_manager(instance, args...);
  return instance;
}
}// namespace oe