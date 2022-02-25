#pragma once

#include <OeCore/ILighting_manager.h>

#include "D3D12_renderer_data.h"

#include <vectormath.hpp>

namespace oe {
class ITexture_manager;

class D3D12_lighting_manager : public ILighting_manager
    , public Manager_base {
 public:
  explicit D3D12_lighting_manager(ITexture_manager& textureManager)
      : ILighting_manager()
      , Manager_base()
      , _textureManager(textureManager)
  {}

  // Manager_base implementations
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override
  {
    return _name;
  }

  // ILighting_manager implementation
  void addEnvironmentVolume(Environment_volume& volume) override;
  void setCurrentVolumeEnvironmentLighting(const Vector3& position) override;
  const Environment_volume::EnvironmentIBL& getEnvironmentLighting() override
  {
    return _environmentIbl;
  }
  Render_light_data_impl<8>* getRenderLightDataLit() override
  {
    return _renderLightData_lit.get();
  }
  Render_light_data_impl<0>* getRenderLightDataUnlit() override
  {
    return _renderLightData_unlit.get();
  }

  static void initStatics();
  static void destroyStatics();

 private:
  ITexture_manager& _textureManager;

  Environment_volume _environment = {};
  Environment_volume::EnvironmentIBL _environmentIbl = {};

  // The template arguments here must match the size of the lights array in the shader constant
  // buffer files.
  std::unique_ptr<D3D12_render_light_data<0>> _renderLightData_unlit;
  std::unique_ptr<D3D12_render_light_data<8>> _renderLightData_lit;

  static std::string _name;
};
}// namespace oe
