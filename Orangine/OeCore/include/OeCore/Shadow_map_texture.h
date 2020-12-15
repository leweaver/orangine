#pragma once

#include "Collision.h"
#include "Texture.h"

namespace oe {

struct Shadow_map_data {
  BoundingOrientedBox boundingOrientedBox; // previously 'castingVolume'
  SSE::Matrix4 worldViewProjMatrix;

  std::shared_ptr<Texture> shadowMap;
};

} // namespace oe