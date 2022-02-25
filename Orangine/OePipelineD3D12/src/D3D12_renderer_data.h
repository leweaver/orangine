#pragma once

#include <OeCore/Renderer_data.h>

#include <d3d12.h>

namespace oe {
template <uint8_t TMax_lights>
class D3D12_render_light_data : public Render_light_data_impl<TMax_lights> {
 public:
  explicit D3D12_render_light_data(ID3D12Device* device) {
    throw std::runtime_error("Not implemented");
  }
};
}