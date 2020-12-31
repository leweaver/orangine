#pragma once

#include "Color.h"
#include "Component.h"

#include "OeCore/Shadow_map_texture.h"

namespace oe {
class Light_component : public Component {
 public:
  explicit Light_component(Entity& entity) : Component(entity) {}
  ~Light_component();

  virtual const Color& color() const = 0;
  virtual void setColor(const Color& color) = 0;

  virtual float intensity() const = 0;
  virtual void setIntensity(float intensity) = 0;

  // Runtime, non-serializable
  std::unique_ptr<Shadow_map_data>& shadowData() { return _shadowMapData; }

 private:
  std::unique_ptr<Shadow_map_data> _shadowMapData;
};

class Directional_light_component : public Light_component {
  DECLARE_COMPONENT_TYPE;

 public:
  Directional_light_component(Entity& entity) : Light_component(entity) {}

  const Color& color() const override { return _component_properties.color; }
  void setColor(const Color& color) override { _component_properties.color = color; }

  float intensity() const override { return _component_properties.intensity; }
  void setIntensity(float intensity) override { _component_properties.intensity = intensity; }

  bool shadowsEnabled() const { return _component_properties.shadowsEnabled; }
  void setShadowsEnabled(bool shadowsEnabled) {
    _component_properties.shadowsEnabled = shadowsEnabled;
  }

  float shadowMapBias() const { return _component_properties.shadowMapBias; }
  void setShadowMapBias(float shadowMapBias) {
    _component_properties.shadowMapBias = shadowMapBias;
  }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  Color color = Colors::White;
  float intensity = 1.0f;
  bool shadowsEnabled = false;
  float shadowMapBias = 0.1f;
  END_COMPONENT_PROPERTIES();
};

class Point_light_component : public Light_component {
  DECLARE_COMPONENT_TYPE;

 public:
  Point_light_component(Entity& entity) : Light_component(entity) {}

  const Color& color() const override { return _component_properties.color; }
  void setColor(const Color& color) override { _component_properties.color = color; }

  float intensity() const override { return _component_properties.intensity; }
  void setIntensity(float intensity) override { _component_properties.intensity = intensity; }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  Color color = Colors::White;
  float intensity = 1.0f;
  END_COMPONENT_PROPERTIES();
};

class Ambient_light_component : public Light_component {
  DECLARE_COMPONENT_TYPE;

 public:
  Ambient_light_component(Entity& entity) : Light_component(entity) {}

  const Color& color() const override { return _component_properties.color; }
  void setColor(const Color& color) override { _component_properties.color = color; }

  float intensity() const override { return _component_properties.intensity; }
  void setIntensity(float intensity) override { _component_properties.intensity = intensity; }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  Color color = Colors::White;
  float intensity = 1.0f;
  END_COMPONENT_PROPERTIES();
};
} // namespace oe
