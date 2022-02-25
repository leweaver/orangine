#include <OeCore/OeCore.h>

#include "Entity_repository.h"
#include "Scene_graph_manager.h"

#include <memory>

using oe::core::Manager_instances;

namespace oe::core {
void create(Manager_instances& managerInstances) {
  // Create all the managers as independent variables rather than assigning to the Manager_instances struct immediately.
  // This ensures that the dependency tree is enforced at compile time
  auto entityRepository = std::static_pointer_cast<IEntity_repository>(std::make_shared<Entity_repository>());
  managerInstances.entityRepository = entityRepository.get();

  auto timeStepManager = create_manager_instance<ITime_step_manager>();

  auto sceneGraphManagerImpl = std::make_unique<internal::Scene_graph_manager>(entityRepository);
  managerInstances.componentFactory = sceneGraphManagerImpl.get();

  auto sceneGraphManager = Manager_instance<IScene_graph_manager>(std::move(sceneGraphManagerImpl));
  auto assetManager = create_manager_instance<IAsset_manager>();


  auto behaviorManager = create_manager_instance<IBehavior_manager>(*sceneGraphManager.instance);

  auto animationManager = create_manager_instance<IAnimation_manager>(*sceneGraphManager.instance, *timeStepManager.instance);

  std::get<Manager_instance<IAnimation_manager>>(managerInstances.managers) = std::move(animationManager);
  std::get<Manager_instance<IAsset_manager>>(managerInstances.managers) = std::move(assetManager);
  std::get<Manager_instance<IBehavior_manager>>(managerInstances.managers) = std::move(behaviorManager);
  std::get<Manager_instance<IScene_graph_manager>>(managerInstances.managers) = std::move(sceneGraphManager);
  std::get<Manager_instance<ITime_step_manager>>(managerInstances.managers) = std::move(timeStepManager);
}
}

Manager_instances::Manager_instances() : componentFactory(nullptr), entityRepository(nullptr)
{
  create(*this);
}