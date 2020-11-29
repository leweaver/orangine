#pragma once

#include "Manager_base.h"

#include <memory>

namespace oe {
class Texture;

class IShadowmap_manager
    : public Manager_base
    , public Manager_deviceDependent {
 public:
  IShadowmap_manager(Scene& scene) : Manager_base(scene) {}
  virtual ~IShadowmap_manager() = default;

  virtual std::shared_ptr<Texture> borrowTexture() = 0;
  virtual void returnTexture(std::shared_ptr<Texture> shadowMap) = 0;
  virtual std::shared_ptr<Texture> shadowMapDepthTextureArray() = 0;
  virtual std::shared_ptr<Texture> shadowMapStencilTextureArray() = 0;
};
} // namespace oe