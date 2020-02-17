#pragma once
#include "Manager_base.h"

namespace DirectX {
class CommonStates;
}

namespace oe {
class ID3D_resources_manager
    : public Manager_base
    , public Manager_deviceDependent {
 public:
  explicit ID3D_resources_manager(Scene& scene) : Manager_base(scene) {}

  virtual DirectX::CommonStates& commonStates() const = 0;
  virtual DX::DeviceResources& deviceResources() const = 0;
};
} // namespace oe