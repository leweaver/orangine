#pragma once

#include <OeCore/Renderer_data.h>

#include "Gpu_buffer.h"

#include <d3d12.h>

namespace oe::pipeline_d3d12 {
template <uint8_t TMax_lights>
class D3D12_render_light_data : public Render_light_data_impl<TMax_lights> {
 public:
  explicit D3D12_render_light_data(ID3D12Device6* device) {
    // DirectX constant buffers must be aligned to 16 byte boundaries (vector4)
    static constexpr size_t bufferSize = sizeof(Render_light_data_impl<TMax_lights>::Light_constants);
    static_assert(bufferSize % 16 == 0);

    std::wstring name = L"D3D12_render_light_data<" + std::to_wstring(TMax_lights) + L">";

    _constantBuffer = Gpu_buffer::create(device, name, bufferSize);
  }

 private:
  std::unique_ptr<Gpu_buffer> _constantBuffer;
};
}