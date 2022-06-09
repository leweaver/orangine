#pragma once

#include "Component.h"
#include "Material.h"
#include "Renderer_data.h"

namespace oe {
class Renderable_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  Renderable_component(Entity& entity) : Component(entity), _material(nullptr) {}
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

  const std::weak_ptr<Mesh_gpu_data>& rendererData() const { return _rendererData; }
  void setRendererData(std::weak_ptr<Mesh_gpu_data>&& rendererData) {
    _rendererData = std::move(rendererData);
  }

  Material_context_handle getMaterialContext() const { return _materialContextHandle; }
  void setMaterialContext(Material_context_handle materialContextHandle) {
    _materialContextHandle = materialContextHandle;
  }

 private:
  BEGIN_COMPONENT_PROPERTIES();
  bool visible = true;
  bool wireframe = false;
  bool castShadow = true;
  END_COMPONENT_PROPERTIES();

  // Runtime, non-serializable
  std::weak_ptr<Mesh_gpu_data> _rendererData;
  Material_context_handle _materialContextHandle {};
  std::shared_ptr<Material> _material;
};

} // namespace oe
