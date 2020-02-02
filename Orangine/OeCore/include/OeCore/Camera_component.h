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
  [[nodiscard]] float fov() const { return _component_properties.fov; }
  void setFov(float fov) { _component_properties.fov = fov; }

  [[nodiscard]] float nearPlane() const { return _component_properties.nearPlane; }
  void setNearPlane(float nearPlane) { _component_properties.nearPlane = nearPlane; }

  [[nodiscard]] float farPlane() const { return _component_properties.farPlane; }
  void setFarPlane(float farPlane) { _component_properties.farPlane = farPlane; }

  
 private:
  BEGIN_COMPONENT_PROPERTIES();
  float fov = DEFAULT_FOV;
  float nearPlane = DEFAULT_NEAR_PLANE;
  float farPlane = DEFAULT_FAR_PLANE;
  END_COMPONENT_PROPERTIES();
};
} // namespace oe
