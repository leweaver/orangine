﻿#pragma once

#include <DirectXCollision.h>
#include <vectormath/vectormath.hpp>
#include "Simple_types.h"

namespace oe {

    struct BoundingSphere {
        SSE::Vector3 center;
        float radius;

        BoundingSphere() noexcept : center(0, 0, 0), radius(1.f) {}

        BoundingSphere(const BoundingSphere&) = default;
        BoundingSphere& operator=(const BoundingSphere&) = default;

        BoundingSphere(BoundingSphere&&) = default;
        BoundingSphere& operator=(BoundingSphere&&) = default;

        constexpr BoundingSphere(const SSE::Vector3& center, float radius)
            : center(center), radius(radius) {}

        static BoundingSphere createFromPoints(Float3* points, int numPoints, size_t strideBytes);
        static void createMerged(BoundingSphere& out, const BoundingSphere& input1, const BoundingSphere& input2);
    };

	/*
	 * A partial re-implementation of DirectX::BoundingFrustum that is compatible with right handed coordinate systems.
	 */
	struct BoundingFrustumRH {

		static const size_t CORNER_COUNT = 8;

		DirectX::XMFLOAT3 Origin;            // Origin of the frustum (and projection).
		DirectX::XMFLOAT4 Orientation;       // Quaternion representing rotation.

		float RightSlope;           // Positive X slope (X/Z).
		float LeftSlope;            // Negative X slope.
		float TopSlope;             // Positive Y slope (Y/Z).
		float BottomSlope;          // Negative Y slope.
		float Near, Far;            // Z of the near plane and far plane.

		// Creators
		BoundingFrustumRH() : Origin(0, 0, 0), Orientation(0, 0, 0, 1.f), RightSlope(1.f), LeftSlope(-1.f),
			TopSlope(1.f), BottomSlope(-1.f), Near(0), Far(-1.f) {}
		XM_CONSTEXPR BoundingFrustumRH(_In_ const DirectX::XMFLOAT3& _Origin, _In_ const DirectX::XMFLOAT4& _Orientation,
			_In_ float _RightSlope, _In_ float _LeftSlope, _In_ float _TopSlope, _In_ float _BottomSlope,
			_In_ float _Near, _In_ float _Far)
			: Origin(_Origin), Orientation(_Orientation),
			RightSlope(_RightSlope), LeftSlope(_LeftSlope), TopSlope(_TopSlope), BottomSlope(_BottomSlope),
			Near(_Near), Far(_Far) {}
		BoundingFrustumRH(_In_ const BoundingFrustumRH& fr) = default;
		explicit BoundingFrustumRH(_In_ DirectX::CXMMATRIX Projection) { CreateFromMatrix(*this, Projection); }

		// Methods
		static inline void CreateFromMatrix(BoundingFrustumRH& Out, DirectX::FXMMATRIX Projection);

		DirectX::ContainmentType Contains(const oe::BoundingSphere& sh) const;

		inline void GetCorners(DirectX::XMFLOAT3* Corners) const;
	};
}

#include "Collision.inl"
