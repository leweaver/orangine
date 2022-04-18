#include <OePipelineD3D12/OePipelineD3D12.h>

#include "D3D12_device_resources.h"
#include "D3D12_texture_manager.h"

#include <memory>

using oe::pipeline_d3d12::Manager_instances;

namespace oe::pipeline_d3d12 {
void create(Manager_instances& managerInstances, oe::core::Manager_instances& coreManagerInstances, HWND mainWindow) {
  // Try to create a rendering device
  auto deviceResources = std::make_unique<D3D12_device_resources>(mainWindow);
  if (!deviceResources) {
    LOG(FATAL) << "Failed to create D3D12 device resources.";
  }
  if (!deviceResources->checkSystemSupport(true)) {
    LOG(FATAL) << "Failed to create D3D12 device resources: unsupported system.";
  }
  managerInstances.deviceResources = deviceResources.get();

  // Create all the managers as independent variables rather than assigning to the Manager_instances struct immediately.
  // This ensures that the dependency tree is enforced at compile time

  // Only device dependencies
  IDevice_resources& iDeviceResources = *managerInstances.deviceResources;
  auto userInterfaceManager = create_manager_instance<IUser_interface_manager>(iDeviceResources);
  auto textureManagerPtr = std::make_unique<oe::pipeline_d3d12::D3D12_texture_manager>(*deviceResources);
  auto textureManagerImpl = textureManagerPtr.get();
  auto textureManager = Manager_instance<ITexture_manager>(std::move(textureManagerPtr));

  auto primitiveMeshDataFactory = create_manager_instance<IPrimitive_mesh_data_factory>();

  auto lightingManager = create_manager_instance<ILighting_manager>(*textureManager.instance, *deviceResources);
  auto shadowmapManager = create_manager_instance<IShadowmap_manager>(*textureManager.instance);
  auto inputManager = create_manager_instance<IInput_manager>(*userInterfaceManager.instance);

  auto materialManager = create_manager_instance<IMaterial_manager>(
          coreManagerInstances.getInstance<IAsset_manager>(),
          *textureManager.instance,
          *lightingManager.instance,
          *primitiveMeshDataFactory.instance,
          *deviceResources);

  auto entityRenderManager = create_manager_instance<IEntity_render_manager>(
          *textureManager.instance, *materialManager.instance, *lightingManager.instance, *primitiveMeshDataFactory.instance, *deviceResources);

  auto& sceneGraphManager = coreManagerInstances.getInstance<IScene_graph_manager>();
  auto devToolsManager = create_manager_instance<IDev_tools_manager>(
          sceneGraphManager, *entityRenderManager.instance, *materialManager.instance, *primitiveMeshDataFactory.instance);

  // Pulls everything together and draws pixels
  auto renderStepManager = create_manager_instance<IRender_step_manager>(sceneGraphManager, *devToolsManager.instance,
                                                                         *textureManagerImpl, *shadowmapManager.instance,
                                                                         *entityRenderManager.instance, *lightingManager.instance, *primitiveMeshDataFactory.instance,
                                                                         deviceResources);

  std::get<Manager_instance<IDev_tools_manager>>(managerInstances.managers) = std::move(devToolsManager);
  std::get<Manager_instance<IEntity_render_manager>>(managerInstances.managers) = std::move(entityRenderManager);
  std::get<Manager_instance<IInput_manager>>(managerInstances.managers) = std::move(inputManager);
  std::get<Manager_instance<ILighting_manager>>(managerInstances.managers) = std::move(lightingManager);
  std::get<Manager_instance<IMaterial_manager>>(managerInstances.managers) = std::move(materialManager);
  std::get<Manager_instance<IRender_step_manager>>(managerInstances.managers) = std::move(renderStepManager);
  std::get<Manager_instance<IShadowmap_manager>>(managerInstances.managers) = std::move(shadowmapManager);
  std::get<Manager_instance<ITexture_manager>>(managerInstances.managers) = {
          std::move(textureManager.interfaces), std::move(textureManager.instance)};
  std::get<Manager_instance<IUser_interface_manager>>(managerInstances.managers) = std::move(userInterfaceManager);
  std::get<Manager_instance<IPrimitive_mesh_data_factory>>(managerInstances.managers) = std::move(primitiveMeshDataFactory);
}
}

Manager_instances::Manager_instances(oe::core::Manager_instances& coreManagerInstances, HWND mainWindow)
{
  create(*this, coreManagerInstances, mainWindow);
}