#pragma once

#include <vectormath/vectormath.hpp>
#include <DirectXMath.h>

namespace oe {
	inline SSE::Vector3 LoadVector3(const DirectX::XMFLOAT3& floats) {
		return { floats.x, floats.y, floats.z };
	}
	inline SSE::Vector4 LoadVector4(const DirectX::XMFLOAT4& floats) {
		return { floats.x, floats.y, floats.z, floats.w };
	}
}