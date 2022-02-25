#pragma once

#include <OeCore/OeCore.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/IEntity_render_manager.h>

#include "Statics.h"
#include "IEntity_scripting_manager.h"

namespace oe::scripting {
struct Manager_instances {
  OeScripting_EXPORT explicit Manager_instances(oe::core::Manager_instances& coreManagerInstances, IInput_manager& inputManager, IEntity_render_manager& entityRenderManager);

    // Items that derive from Manager_base
  using Managers_tuple = std::tuple<Manager_instance<IEntity_scripting_manager>>;
  Managers_tuple managers;

  // Helper method
  template<class TManager>
  TManager& getInstance() {
    return *std::get<Manager_instance<TManager>>(managers).instance;
  }
};
}// namespace oe::scripting