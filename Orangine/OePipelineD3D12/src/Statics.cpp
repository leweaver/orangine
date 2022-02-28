#include <OePipelineD3D12/Statics.h>

#include "D3D12_Lighting_manager.h"
#include "D3D12_render_step_manager.h"
#include "Primitive_mesh_data_factory.h"

using namespace oe;
using namespace oe::core;

void oe::pipeline_d3d12::initStatics()
{
  D3D12_lighting_manager::initStatics();
  D3D12_render_step_manager::initStatics();
  Primitive_mesh_data_factory::initStatics();
}

void oe::pipeline_d3d12::destroyStatics()
{
  // Inverse order of init
  Primitive_mesh_data_factory::destroyStatics();
  D3D12_render_step_manager::destroyStatics();
  D3D12_lighting_manager::destroyStatics();
}
