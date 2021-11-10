#include "pch.h"

#include <OeScripting/OeScripting.h>

#include <memory>

namespace oe::scripting {
void create(Manager_instances& managerInstances, oe::core::Manager_instances& coreManagerInstances) {

  auto entityScriptingManager = create_manager_instance<IEntity_scripting_manager>(
          coreManagerInstances.getInstance<ITime_step_manager>(),
          coreManagerInstances.getInstance<IScene_graph_manager>(),
          coreManagerInstances.getInstance<IInput_manager>(),
          coreManagerInstances.getInstance<IAsset_manager>(),
          coreManagerInstances.getInstance<IEntity_render_manager>());

  std::get<Manager_instance<IEntity_scripting_manager>>(managerInstances.managers) = std::move(entityScriptingManager);
}
}

oe::scripting::Manager_instances::Manager_instances(oe::core::Manager_instances& coreManagerInstances)
{
  create(*this, coreManagerInstances);
}