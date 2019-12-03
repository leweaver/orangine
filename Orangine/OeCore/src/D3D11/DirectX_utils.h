#pragma once

#include <vectormath/vectormath.hpp>
#include <DirectXMath.h>

namespace oe {
	inline SSE::Vector3 LoadVector3(const DirectX::XMFLOAT3& floats) {
		return { floats.x, floats.y, floats.z };
	}
    inline DirectX::XMFLOAT3 StoreVector3(const SSE::Vector3& vec) {
        return { vec.getX(), vec.getY(), vec.getZ() };
    }
	inline SSE::Vector4 LoadVector4(const DirectX::XMFLOAT4& floats) {
		return { floats.x, floats.y, floats.z, floats.w };
	}
    inline DirectX::XMFLOAT4 StoreVector4(const SSE::Vector4& vec) {
        return { vec.getX(), vec.getY(), vec.getZ(), vec.getW() };
    }
    inline SSE::Quat LoadQuat(const DirectX::XMFLOAT4& floats) {
        return { floats.x, floats.y, floats.z, floats.w };
    }
    inline DirectX::XMFLOAT4 StoreQuat(const SSE::Quat& vec) {
        return { vec.getX(), vec.getY(), vec.getZ(), vec.getW() };
    }
    inline DirectX::BoundingSphere StoreBoundingSphere(const oe::BoundingSphere& bs) {
        return DirectX::BoundingSphere(
            StoreVector3(bs.center),
            bs.radius
        );
    }
}