#pragma once

#include "Component.h"
#include "Material.h"
#include "Material_context.h"

// TODO: Remove this somehow
#include "D3D11/D3D_renderer_data.h"

namespace oe {
class Renderable_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  Renderable_component(Entity& entity)
      : Component(entity)
      , _rendererData(nullptr)
      , _material(nullptr) {}
  ~Renderable_component();

  bool visible() const { return _component_properties.visible; }
  void setVisible(bool visible) { _component_properties.visible = visible; }

  bool wireframe() const { return _component_properties.wireframe; }
  void setWireframe(bool wireframe) { _component_properties.wireframe = wireframe; }

  bool castShadow() const { return _component_properties.castShadow; }
  void setCastShadow(bool castShadow) { _component_properties.castShadow = castShadow; }

  // Runtime, non-serializable
  const std::shared_ptr<Material>& material() const { return _material; }
  void setMaterial(std::shared_ptr<Material> material) { _material = material; }

  const std::unique_ptr<Renderer_data>& rendererData() const { return _rendererData; }
  void setRendererData(std::unique_ptr<Renderer_data>&& rendererData) {
    _rendererData = std::move(rendererData);
  }

  const std::weak_ptr<Material_context>& materialContext() const { return _materialContext; }
  void setMaterialContext(std::weak_ptr<Material_context>&& materialContext) {
    _materialContext = std::move(materialContext);
  }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  bool visible = true;
  bool wireframe = false;
  bool castShadow = true;
  END_COMPONENT_PROPERTIES();

  // Runtime, non-serializable
  std::unique_ptr<Renderer_data> _rendererData;
  std::weak_ptr<Material_context> _materialContext;
  std::shared_ptr<Material> _material;
};

} // namespace oe
