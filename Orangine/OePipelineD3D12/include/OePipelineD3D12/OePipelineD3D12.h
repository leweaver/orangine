#pragma once

#include <OePipelineD3D12/OePipelineD3D12_Export.h>
#include <OePipelineD3D12/Statics.h>

#include <OeCore/WindowsDefines.h>
#include <OeCore/IDev_tools_manager.h>
#include <OeCore/IDevice_resources.h>
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IInput_manager.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/IMaterial_manager.h>
#include <OeCore/IRender_step_manager.h>
#include <OeCore/IShadowmap_manager.h>
#include <OeCore/ITexture_manager.h>
#include <OeCore/IUser_interface_manager.h>

namespace oe::pipeline_d3d12 {
struct Manager_instances {
  OePipelineD3D12_EXPORT Manager_instances(oe::core::Manager_instances& coreManagerInstances, HWND mainWindow);

  IDevice_resources* deviceResources;

  // Items that derive from Manager_base
  using Managers_tuple = std::tuple<
          Manager_instance<IDev_tools_manager>, Manager_instance<IEntity_render_manager>,
          Manager_instance<IInput_manager>, Manager_instance<ILighting_manager>, Manager_instance<IMaterial_manager>,
          Manager_instance<IRender_step_manager>, Manager_instance<IShadowmap_manager>,
          Manager_instance<ITexture_manager>, Manager_instance<IUser_interface_manager>,
          Manager_instance<IPrimitive_mesh_data_factory>>;
  Managers_tuple managers;

  // Helper method
  template<class TManager> TManager& getInstance()
  {
    if constexpr (std::is_same_v<TManager, IDevice_resources>) {
      return *deviceResources;
    }
    else
    {
      return *std::get<Manager_instance<TManager>>(managers).instance;
    }
  }
};
}