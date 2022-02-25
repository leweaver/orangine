#pragma once

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

/*
#include <DirectXHelpers.h>
*/
#include <DirectXColors.h>
#include <DirectXMath.h>

// Microsoft DirectX12 Helpers
#include <d3dx12/d3dx12.h>


#ifdef _DEBUG
#include <dxgidebug.h>
#endif

inline SSE::Vector3 LoadVector3(const DirectX::XMFLOAT3& floats) {
  return {floats.x, floats.y, floats.z};
}
inline DirectX::XMFLOAT3 StoreVector3(const SSE::Vector3& vec) {
  return {vec.getX(), vec.getY(), vec.getZ()};
}
inline SSE::Vector4 LoadVector4(const DirectX::XMFLOAT4& floats) {
  return {floats.x, floats.y, floats.z, floats.w};
}
inline DirectX::XMFLOAT4 StoreVector4(const SSE::Vector4& vec) {
  return {vec.getX(), vec.getY(), vec.getZ(), vec.getW()};
}
inline SSE::Quat LoadQuat(const DirectX::XMFLOAT4& floats) {
  return {floats.x, floats.y, floats.z, floats.w};
}
inline DirectX::XMFLOAT4 StoreQuat(const SSE::Quat& vec) {
  return {vec.getX(), vec.getY(), vec.getZ(), vec.getW()};
}