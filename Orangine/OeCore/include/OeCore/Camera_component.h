#pragma once
#include <OeCore/Component.h>

namespace oe {
class Camera_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  static const float DEFAULT_FOV;
  static const float DEFAULT_NEAR_PLANE;
  static const float DEFAULT_FAR_PLANE;

  explicit Camera_component(Entity& entity) : Component(entity) {}

  // Field of view, in radians
  [[nodiscard]] float fov() const { return _fov; }
  void setFov(float fov) { _fov = fov; }

  [[nodiscard]] float nearPlane() const { return _nearPlane; }
  void setNearPlane(float nearPlane) { _nearPlane = nearPlane; }

  [[nodiscard]] float farPlane() const { return _farPlane; }
  void setFarPlane(float farPlane) { _farPlane = farPlane; }

  
 private:
  float _fov = DEFAULT_FOV;
  float _nearPlane = DEFAULT_NEAR_PLANE;
  float _farPlane = DEFAULT_FAR_PLANE;
};
} // namespace oe
