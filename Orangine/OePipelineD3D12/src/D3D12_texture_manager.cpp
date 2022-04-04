//
// Created by Lewis weaver on 3/20/2022.
//

#include "D3D12_texture_manager.h"

namespace oe::pipeline_d3d12 {

void D3D12_texture_manager::initStatics()
{
  D3D12_texture_manager::_name = "D3D12_texture_manager";
}

void D3D12_texture_manager::destroyStatics() {}
}