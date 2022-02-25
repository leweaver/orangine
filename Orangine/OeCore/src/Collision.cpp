#include "OeCore/Collision.h"

bool oe::intersect_ray_sphere(
    const oe::Ray& ray,
    const oe::BoundingSphere& sphere,
    Ray_intersection& intersection) {
  const SSE::Vector3 midpoint = ray.origin - SSE::Point3(sphere.center);
  const float b = SSE::dot(midpoint, ray.directionNormal);
  const float c = SSE::dot(midpoint, midpoint) - sphere.radius * sphere.radius;
  if (c > 0.0f && b > 0.0f) {
    // rays origin outside sphere, and pointing away.
    return false;
  }
  const float disc = b * b - c;
  if (disc < 0.0f) {
    // Negative discriminant corresponds to ray missing the sphere
    return false;
  }

  const float t = -b - sqrtf(disc);
  if (t < 0.0f) {
    // ray started inside the sphere
    intersection.position = ray.origin;
    intersection.distance = 0.0f;
    return true;
  }

  intersection.position = ray.origin + ray.directionNormal * t;
  intersection.distance = t;
  return true;
}

bool oe::test_ray_sphere(const oe::Ray& ray, const oe::BoundingSphere& sphere) {
  const SSE::Vector3 midpoint = ray.origin - SSE::Point3(sphere.center);
  const float c = SSE::dot(midpoint, midpoint) - sphere.radius * sphere.radius;
  if (c < 0.0f) {
    // Start is inside the sphere, so there is at least 1 real root: must be an intersection.
    return true;
  }
  const float b = SSE::dot(midpoint, ray.directionNormal);
  if (b > 0.0f) {
    // Ray starts outside sphere, and points away from it.
    return false;
  }
  const float disc = b * b - c;
  if (disc < 0.0f) {
    // Negative discriminant corresponds to ray missing the sphere
    return false;
  }
  return true;
}