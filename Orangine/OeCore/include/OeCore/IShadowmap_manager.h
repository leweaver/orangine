#pragma once

#include "Manager_base.h"

#include <memory>

namespace oe {
class Texture;

class IShadowmap_manager {
 public:
  virtual ~IShadowmap_manager() = default;

  virtual std::shared_ptr<Texture> borrowTextureSlice() = 0;
  virtual void returnTextureSlice(std::shared_ptr<Texture> shadowMap) = 0;
  virtual std::shared_ptr<Texture> getShadowMapTextureArray() = 0;
};
} // namespace oe