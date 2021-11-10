#pragma once

#include <OeCore/Manager_base.h>

#include <vector>

namespace oe::app {

class Manager_collection {
 public:
  template<class TInterface> struct Manager_interface_config {
    Manager_interface_config(Manager_interfaces* config, TInterface instance)
        : config(config)
        , instance(instance)
    {}
    Manager_interfaces* config;
    TInterface instance;
    TInterface operator() () const
    {
      return instance;
    }
  };

  template<class TClass> void addManager(TClass* instance) {
    addManager(Manager_interfaces::create(instance));
  }

  void addManager(const Manager_interfaces& srcConfig)
  {
    if (srcConfig.asBase == nullptr) {
      OE_THROW(std::invalid_argument("config.asBase must nut be nullptr."));
    }

    auto config = std::make_unique<Manager_interfaces>(srcConfig);
    if (config->asTickable) {
      _tickableManagers.emplace_back(config.get(), config->asTickable);
    }
    if (config->asDeviceDependent) {
      _deviceDependentManagers.emplace_back(config.get(), config->asDeviceDependent);
    }
    if (config->asWindowDependent) {
      _windowDependentManagers.emplace_back(config.get(), config->asWindowDependent);
    }
    if (config->asWindowsMessageProcessor) {
      _windowsMessageProcessorManagers.emplace_back(config.get(), config->asWindowsMessageProcessor);
    }
    _allManagers.push_back(std::move(config));
  }

  const std::vector<std::unique_ptr<Manager_interfaces>>& getAllManagers() const
  {
    return _allManagers;
  }
  const std::vector<Manager_interface_config<Manager_tickable*>>& getTickableManagers() const
  {
    return _tickableManagers;
  }
  const std::vector<Manager_interface_config<Manager_windowDependent*>>& getWindowDependentManagers() const
  {
    return _windowDependentManagers;
  }
  const std::vector<Manager_interface_config<Manager_deviceDependent*>>& getDeviceDependentManagers() const
  {
    return _deviceDependentManagers;
  }
  const std::vector<Manager_interface_config<Manager_windowsMessageProcessor*>>&
  getWindowsMessageProcessorManagers() const
  {
    return _windowsMessageProcessorManagers;
  }

 private:
  std::vector<std::unique_ptr<Manager_interfaces>> _allManagers;
  std::vector<Manager_interface_config<Manager_tickable*>> _tickableManagers;
  std::vector<Manager_interface_config<Manager_windowDependent*>> _windowDependentManagers;
  std::vector<Manager_interface_config<Manager_deviceDependent*>> _deviceDependentManagers;
  std::vector<Manager_interface_config<Manager_windowsMessageProcessor*>> _windowsMessageProcessorManagers;
};
}