#pragma once

#include <memory>

namespace oe {
class Texture;

class Environment_volume {
 public:
  struct EnvironmentIBL {
    std::shared_ptr<Texture> skyboxTexture;
    std::shared_ptr<Texture> iblBrdfTexture;
    std::shared_ptr<Texture> iblDiffuseTexture;
    std::shared_ptr<Texture> iblSpecularTexture;
  } environmentIbl = {};
};
}
