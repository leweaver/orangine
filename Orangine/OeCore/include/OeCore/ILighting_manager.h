#pragma once

#include "Manager_base.h"

#include "Render_light_data.h"
#include "Environment_volume.h"

namespace oe {

class ILighting_manager {
 public:
  // ILighting_manager
  virtual void addEnvironmentVolume(Environment_volume& volume) = 0;
  virtual void setCurrentVolumeEnvironmentLighting(const Vector3& position) = 0;
  virtual const Environment_volume::EnvironmentIBL& getEnvironmentLighting() = 0;

  // The template arguments here must match the size of the lights array in the shader constant
  // buffer files.
  virtual Render_light_data_impl<0>* getRenderLightDataUnlit() = 0;
  virtual Render_light_data_impl<8>* getRenderLightDataLit() = 0;
};
}