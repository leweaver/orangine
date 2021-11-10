#pragma once

#include "Manager_base.h"

#include <memory>

namespace oe {
class Texture;

class IShadowmap_manager {
 public:
  virtual ~IShadowmap_manager() = default;

  virtual std::shared_ptr<Texture> borrowTexture() = 0;
  virtual void returnTexture(std::shared_ptr<Texture> shadowMap) = 0;
  virtual std::shared_ptr<Texture> shadowMapDepthTextureArray() = 0;
  virtual std::shared_ptr<Texture> shadowMapStencilTextureArray() = 0;
};
} // namespace oe