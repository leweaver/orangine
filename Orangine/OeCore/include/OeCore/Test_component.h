#pragma once

#include "Component.h"

namespace oe {
class Entity;

class Test_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  Test_component(Entity& entity) : Component(entity) {}

  void setSpeed(const SSE::Vector3& speed) { _component_properties.speed = speed; }
  const SSE::Vector3& getSpeed() const { return _component_properties.speed; }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  SSE::Vector3 speed = SSE::Vector3(0);
  END_COMPONENT_PROPERTIES();
};
} // namespace oe