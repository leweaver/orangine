#pragma once

#include "OeCore/Renderer_types.h"

#include <DirectXCollision.h>
#include <vectormath/vectormath.hpp>

namespace oe {

struct Float3;

struct Ray {
  SSE::Point3 origin;
  SSE::Vector3 directionNormal;
};

struct BoundingSphere {
  SSE::Vector3 center;
  float radius;

  BoundingSphere() noexcept : center(0, 0, 0), radius(0.f) {}

  BoundingSphere(const BoundingSphere&) = default;
  BoundingSphere& operator=(const BoundingSphere&) = default;

  BoundingSphere(BoundingSphere&&) = default;
  BoundingSphere& operator=(BoundingSphere&&) = default;

  constexpr BoundingSphere(const SSE::Vector3& center, float radius)
      : center(center), radius(radius)
  {
  }

  static BoundingSphere createFromPoints(Float3* points, int numPoints, size_t strideBytes);
  static void createMerged(BoundingSphere& out, const BoundingSphere& input1,
                           const BoundingSphere& input2);
};

//-------------------------------------------------------------------------------------
// Oriented bounding box
//-------------------------------------------------------------------------------------
struct BoundingOrientedBox {
  static const size_t CORNER_COUNT = 8;

  SSE::Vector3 center;   // Center of the box.
  SSE::Vector3 extents;  // Distance from the center to each side.
  SSE::Quat orientation; // Unit quaternion representing rotation (box -> world).

  // Creators
  BoundingOrientedBox() noexcept
      : center(0, 0, 0), extents(1.f, 1.f, 1.f), orientation(0, 0, 0, 1.f)
  {
  }

  BoundingOrientedBox(const BoundingOrientedBox&) = default;
  BoundingOrientedBox& operator=(const BoundingOrientedBox&) = default;

  BoundingOrientedBox(BoundingOrientedBox&&) = default;
  BoundingOrientedBox& operator=(BoundingOrientedBox&&) = default;

  constexpr BoundingOrientedBox(const SSE::Vector3& center, const SSE::Vector3& extents,
                                const SSE::Quat& orientation)
      : center(center), extents(extents), orientation(orientation)
  {
  }

  // Methods
  bool contains(const BoundingSphere& boundingSphere) const;
};

/*
 * A partial re-implementation of DirectX::BoundingFrustum that is compatible with right handed
 * coordinate systems.
 */
struct BoundingFrustumRH {

  static const size_t CORNER_COUNT = 8;

  SSE::Vector3 origin;   // Origin of the frustum (and projection).
  SSE::Quat orientation; // Quaternion representing rotation.

  float rightSlope;          // Positive X slope (X/Z).
  float leftSlope;           // Negative X slope.
  float topSlope;            // Positive Y slope (Y/Z).
  float bottomSlope;         // Negative Y slope.
  float nearPlane, farPlane; // Z of the near plane and far plane.

  // Creators
  BoundingFrustumRH()
      : origin(0, 0, 0), orientation(0, 0, 0, 1.f), rightSlope(1.f), leftSlope(-1.f), topSlope(1.f),
        bottomSlope(-1.f), nearPlane(0), farPlane(-1.f)
  {
  }
  constexpr BoundingFrustumRH(const SSE::Vector3& _origin, const SSE::Quat& _orientation,
                              float _rightSlope, float _leftSlope, float _topSlope,
                              float _bottomSlope, float _near, float _far)
      : origin(_origin), orientation(_orientation), rightSlope(_rightSlope), leftSlope(_leftSlope),
        topSlope(_topSlope), bottomSlope(_bottomSlope), nearPlane(_near), farPlane(_far)
  {
  }
  BoundingFrustumRH(const BoundingFrustumRH& fr) = default;
  explicit BoundingFrustumRH(const SSE::Matrix4& projection)
  {
    CreateFromMatrix(*this, projection);
  }

  // Methods
  static void CreateFromMatrix(BoundingFrustumRH& Out, const SSE::Matrix4& Projection);

  DirectX::ContainmentType Contains(const oe::BoundingSphere& sh) const;

  void GetCorners(SSE::Vector3* Corners) const;
};

struct Ray_intersection {
  SSE::Point3 position = {0.0f, 0.0f, 0.0f};
  float distance = 0.0f;
};

// Ref: Real-time collision detection, Christer Ericson
bool intersect_ray_sphere(const Ray& ray, const BoundingSphere& sphere,
                          Ray_intersection& intersection);
bool test_ray_sphere(const Ray& ray, const BoundingSphere& sphere);
} // namespace oe

#include "Collision.inl"
