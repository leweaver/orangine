#pragma once

#include <OeCore/Statics.h>

#include <OeCore/IAnimation_manager.h>
#include <OeCore/IAsset_manager.h>
#include <OeCore/IBehavior_manager.h>
#include <OeCore/IConfigReader.h>
#include <OeCore/IDev_tools_manager.h>
#include <OeCore/IDevice_resources.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IEntity_repository.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/IMaterial_manager.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/IScene_graph_manager.h>
#include <OeCore/IShadowmap_manager.h>
#include <OeCore/ITexture_manager.h>
#include <OeCore/ITime_step_manager.h>
#include <OeCore/IUser_interface_manager.h>

namespace oe::core {
struct Manager_instances {
  explicit Manager_instances(IDevice_resources& deviceResources);

  // Helper method
  template<class TManager> TManager& getInstance()
  {
    if constexpr (std::is_same_v<TManager, IComponent_factory>) {
      return *componentFactory;
    }
    else if constexpr (std::is_same_v<TManager, IEntity_repository>) {
      return *entityRepository;
    }
    else
    {
      return *std::get<Manager_instance<TManager>>(managers).instance;
    }
  }

  // Misc types that are not derived from Manager_base. Their lifetime is tied to the lifetime of the other managers.
  IComponent_factory* componentFactory;
  IEntity_repository* entityRepository;

  // Items that derive from Manager_base
  using Managers_tuple = std::tuple<
          Manager_instance<IAnimation_manager>, Manager_instance<IAsset_manager>, Manager_instance<IBehavior_manager>,
          Manager_instance<IDev_tools_manager>, Manager_instance<IEntity_render_manager>,
          Manager_instance<IInput_manager>, Manager_instance<ILighting_manager>, Manager_instance<IMaterial_manager>,
          Manager_instance<IRender_step_manager>, Manager_instance<IScene_graph_manager>,
          Manager_instance<IShadowmap_manager>, Manager_instance<ITexture_manager>,
          Manager_instance<ITime_step_manager>, Manager_instance<IUser_interface_manager>>;
  Managers_tuple managers;
};

template<typename TTuple, int TIdx = 0>
void for_each_manager_instance(const TTuple& instances, std::function<void(const oe::Manager_interfaces&)> fn) {
  fn(std::get<TIdx>(instances).interfaces);

  // iterate the next manager
  if constexpr (TIdx + 1 < std::tuple_size_v<TTuple>) {
    for_each_manager_instance<TTuple, TIdx + 1>(instances, fn);
  }
}

}// namespace oe::core