#pragma once

#include "Renderer_types.h"

#include <array>
#include <vectormath.hpp>

namespace oe {

// This is declared in internal renderer classes, and contains internal data only.
struct Renderer_data;

struct Renderer_animation_data {
  static constexpr size_t morphWeightsSize = 8;
  std::array<float, morphWeightsSize> morphWeights;

  uint32_t numBoneTransforms = 0;
  std::array<SSE::Matrix4, g_max_bone_transforms> boneTransformConstants;
};

// NOTE: When adding new items to this, make sure to also add them to the hashing function
struct Renderer_features_enabled {
  bool vertexMorph = true;
  bool skinnedAnimation = true;

  Debug_display_mode debugDisplayMode = Debug_display_mode::None;
  bool shadowsEnabled = true;
  bool irradianceMappingEnabled = true;
  bool enableShaderOptimization = false;
  size_t hash() const;
};
} // namespace oe
