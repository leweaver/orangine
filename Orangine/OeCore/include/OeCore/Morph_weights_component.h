#pragma once

#include "OeCore/Component.h"
#include <array>

namespace oe {
class Morph_weights_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  Morph_weights_component(Entity& entity) : Component(entity) {}

  static uint8_t maxMorphTargetCount() { return 8; }
  uint8_t morphTargetCount() const { return _component_properties.morphTargetCount; }
  void setMorphTargetCount(uint8_t morphTargetCount) { _component_properties.morphTargetCount = morphTargetCount; }

  const std::array<double, 8>& morphWeights() const { return _component_properties.morphWeights; }
  std::array<double, 8>& morphWeights() { return _component_properties.morphWeights; }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  std::array<double, 8> morphWeights;
  uint8_t morphTargetCount = 0;
  END_COMPONENT_PROPERTIES();
};
} // namespace oe