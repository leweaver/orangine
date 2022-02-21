#include <OeCore/Statics.h>
#include <OeCore/Component.h>

#include <OeCore/Animation_controller_component.h>
#include <OeCore/Camera_component.h>
#include <OeCore/Light_component.h>
#include <OeCore/Mesh_data_component.h>
#include <OeCore/Morph_weights_component.h>
#include <OeCore/Renderable_component.h>
#include <OeCore/Skinned_mesh_component.h>
#include <OeCore/Test_component.h>

#include "D3D12/D3D12_lighting_manager.h"
#include "D3D12/D3D12_render_step_manager.h"

using namespace oe;
using namespace oe::core;

void oe::core::initStatics() {
  // Must be initialized before the other components
  Component_factory::initStatics();

  Animation_controller_component::initStatics();
  Camera_component::initStatics();
  Directional_light_component::initStatics();
  Point_light_component::initStatics();
  Ambient_light_component::initStatics();
  Mesh_data_component::initStatics();
  Morph_weights_component::initStatics();
  Renderable_component::initStatics();
  Skinned_mesh_component::initStatics();
  Test_component::initStatics();

  D3D12_lighting_manager::initStatics();
  D3D12_render_step_manager::initStatics();
}

void oe::core::destroyStatics()
{
  // Inverse order of init
  D3D12_render_step_manager::destroyStatics();
  D3D12_lighting_manager::destroyStatics();

  Test_component::destroyStatics();
  Skinned_mesh_component::destroyStatics();
  Renderable_component::destroyStatics();
  Morph_weights_component::destroyStatics();
  Mesh_data_component::destroyStatics();
  Ambient_light_component::destroyStatics();
  Point_light_component::destroyStatics();
  Directional_light_component::destroyStatics();
  Camera_component::destroyStatics();
  Animation_controller_component::destroyStatics();

  Component_factory::destroyStatics();
}
