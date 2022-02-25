#include <OeCore/OeCore.h>

#include "Entity_repository.h"
#include "Scene_graph_manager.h"

#include <memory>

using oe::core::Manager_instances;

namespace oe::core {
void create(Manager_instances& managerInstances, IDevice_resources& deviceResources) {
  // Create all the managers as independent variables rather than assigning to the Manager_instances struct immediately.
  // This ensures that the dependency tree is enforced at compile time
  auto entityRepository = std::static_pointer_cast<IEntity_repository>(std::make_shared<Entity_repository>());
  managerInstances.entityRepository = entityRepository.get();

  auto timeStepManager = create_manager_instance<ITime_step_manager>();

  auto sceneGraphManagerImpl = std::make_unique<internal::Scene_graph_manager>(entityRepository);
  managerInstances.componentFactory = sceneGraphManagerImpl.get();

  auto sceneGraphManager = Manager_instance<IScene_graph_manager>(std::move(sceneGraphManagerImpl));
  auto assetManager = create_manager_instance<IAsset_manager>();

  // Only device dependencies
  auto userInterfaceManager = create_manager_instance<IUser_interface_manager>(deviceResources);
  auto textureManager = create_manager_instance<ITexture_manager>(deviceResources);

  auto behaviorManager = create_manager_instance<IBehavior_manager>(*sceneGraphManager.instance);
  auto materialManager = create_manager_instance<IMaterial_manager>(*assetManager.instance);
  auto lightingManager = create_manager_instance<ILighting_manager>(*textureManager.instance);
  auto shadowmapManager = create_manager_instance<IShadowmap_manager>(*textureManager.instance);
  auto inputManager = create_manager_instance<IInput_manager>(*userInterfaceManager.instance);

  auto animationManager = create_manager_instance<IAnimation_manager>(*sceneGraphManager.instance, *timeStepManager.instance);
  auto entityRenderManager = create_manager_instance<IEntity_render_manager>(
          *textureManager.instance, *materialManager.instance, *lightingManager.instance);

  auto devToolsManager = create_manager_instance<IDev_tools_manager>(
          *sceneGraphManager.instance, *entityRenderManager.instance, *materialManager.instance);

  // Pulls everything together and draws pixels
  auto renderStepManager = create_manager_instance<IRender_step_manager>(*sceneGraphManager.instance, *devToolsManager.instance,
                                                                         *textureManager.instance, *shadowmapManager.instance,
                                                                         *entityRenderManager.instance, *lightingManager.instance);

  std::get<Manager_instance<IAnimation_manager>>(managerInstances.managers) = std::move(animationManager);
  std::get<Manager_instance<IAsset_manager>>(managerInstances.managers) = std::move(assetManager);
  std::get<Manager_instance<IBehavior_manager>>(managerInstances.managers) = std::move(behaviorManager);
  std::get<Manager_instance<IDev_tools_manager>>(managerInstances.managers) = std::move(devToolsManager);
  std::get<Manager_instance<IEntity_render_manager>>(managerInstances.managers) = std::move(entityRenderManager);
  std::get<Manager_instance<IInput_manager>>(managerInstances.managers) = std::move(inputManager);
  std::get<Manager_instance<ILighting_manager>>(managerInstances.managers) = std::move(lightingManager);
  std::get<Manager_instance<IMaterial_manager>>(managerInstances.managers) = std::move(materialManager);
  std::get<Manager_instance<IRender_step_manager>>(managerInstances.managers) = std::move(renderStepManager);
  std::get<Manager_instance<IScene_graph_manager>>(managerInstances.managers) = std::move(sceneGraphManager);
  std::get<Manager_instance<IShadowmap_manager>>(managerInstances.managers) = std::move(shadowmapManager);
  std::get<Manager_instance<ITexture_manager>>(managerInstances.managers) = std::move(textureManager);
  std::get<Manager_instance<ITime_step_manager>>(managerInstances.managers) = std::move(timeStepManager);
  std::get<Manager_instance<IUser_interface_manager>>(managerInstances.managers) = std::move(userInterfaceManager);
}
}

Manager_instances::Manager_instances(IDevice_resources& deviceResources) : componentFactory(nullptr), entityRepository(nullptr)
{
  create(*this, deviceResources);
}