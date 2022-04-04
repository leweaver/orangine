#pragma once

#include "OeCore/EngineUtils.h"
#include <cstdio>
#include <d3d12.h>

namespace oe::pipeline_d3d12 {
#if defined(NDEBUG)
inline void SetObjectName(ID3D12Object*, LPCWSTR) {}
inline void SetObjectName(ID3D12Object*, LPCSTR) {}
inline void SetObjectNameIndexed(ID3D12Object*, LPCWSTR, UINT) {}
inline void SetObjectNameIndexed(ID3D12Object*, LPCSTR, UINT) {}
#else
inline void SetObjectName(ID3D12Object* pObject, LPCWSTR name)
{
  pObject->SetName(name);
}
inline void SetObjectName(ID3D12Object* pObject, LPCSTR name)
{
  SetObjectName(pObject, oe::utf8_decode(name).c_str());
}
inline void SetObjectNameIndexed(ID3D12Object* pObject, LPCWSTR name, uint32_t index)
{
  if (name == nullptr) {
    return;
  }
  std::wstringstream ss;
  ss << name << "[" << index << "]";
  pObject->SetName(ss.str().c_str());
}
inline void SetObjectNameIndexed(ID3D12Object* pObject, LPCSTR name, uint32_t index)
{
  SetObjectNameIndexed(pObject, oe::utf8_decode(name).c_str(), index);
}
#endif

inline D3D12_TEXTURE_ADDRESS_MODE toD3D12TextureAddressMode(Sampler_texture_address_mode textureAddressMode) {
  return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(static_cast<uint32_t>(textureAddressMode) + 1);
}

inline D3D12_COMPARISON_FUNC toD3D12ComparisonFunc(Sampler_comparison_func sampleComparisonFunc) {
  return static_cast<D3D12_COMPARISON_FUNC>(static_cast<uint32_t>(sampleComparisonFunc) + 1);
}

inline D3D12_FILTER toD3DBasicFilter(Sampler_filter_type minFilterType, Sampler_filter_type magFilterType) {
  static constexpr auto numFilterType = static_cast<uint32_t>(Sampler_filter_type::Num_sampler_filter_type);
  // first part of the Sampler_filter_type enum name represents the "min" filtering.
  static constexpr std::array<D3D12_FILTER_TYPE, numFilterType> kTextureMinFilterMinLUT = {{
          D3D12_FILTER_TYPE_POINT, // Point
          D3D12_FILTER_TYPE_LINEAR,// Linear
          D3D12_FILTER_TYPE_POINT, // Point_mipmap_point
          D3D12_FILTER_TYPE_POINT, // Point_mipmap_linear
          D3D12_FILTER_TYPE_LINEAR,// Linear_mipmap_point
          D3D12_FILTER_TYPE_LINEAR,// Linear_mipmap_linear
  }};
  // second part of the Sampler_filter_type (mipmap_x) describes the filtering of mip
  static constexpr std::array<D3D12_FILTER_TYPE, numFilterType> kTextureMinFilterMipLUT = {{
          D3D12_FILTER_TYPE_POINT, // Point
          D3D12_FILTER_TYPE_POINT, // Linear
          D3D12_FILTER_TYPE_POINT, // Point_mipmap_point
          D3D12_FILTER_TYPE_LINEAR,// Point_mipmap_linear
          D3D12_FILTER_TYPE_POINT, // Linear_mipmap_point
          D3D12_FILTER_TYPE_LINEAR,// Linear_mipmap_linear
  }};
  static constexpr std::array<D3D12_FILTER_TYPE, numFilterType> kTextureMagFilterMagLUT = {{
          D3D12_FILTER_TYPE_POINT, // Point
          D3D12_FILTER_TYPE_LINEAR,// Linear
          D3D12_FILTER_TYPE_POINT, // Point_mipmap_point
          D3D12_FILTER_TYPE_POINT, // Point_mipmap_linear
          D3D12_FILTER_TYPE_POINT, // Linear_mipmap_point
          D3D12_FILTER_TYPE_POINT, // Linear_mipmap_linear
  }};
  const auto minFilter = kTextureMinFilterMinLUT.at(static_cast<size_t>(minFilterType));
  const auto magFilter = kTextureMagFilterMagLUT.at(static_cast<size_t>(magFilterType));
  // Take mip filter setting from min
  const auto mipFilter = kTextureMinFilterMipLUT.at(static_cast<size_t>(minFilterType));

  // TODO: support different reduction types.
  return D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

D3D12_SHADER_VISIBILITY toD3dShaderVisibility(Shader_constant_buffer_visibility visibility) {
  switch (visibility) {
    case Shader_constant_buffer_visibility::All:
      return D3D12_SHADER_VISIBILITY_ALL;
    case Shader_constant_buffer_visibility::Vertex:
      return D3D12_SHADER_VISIBILITY_VERTEX;
    case Shader_constant_buffer_visibility::Pixel:
      return D3D12_SHADER_VISIBILITY_PIXEL;
    default:
      OE_CHECK_MSG(false, "Unsupported Shader_constant_buffer_visibility value: " + shaderConstantBufferVisibilityToString(visibility));
      return D3D12_SHADER_VISIBILITY_ALL;
  }
}

}// namespace oe::pipeline_d3d12