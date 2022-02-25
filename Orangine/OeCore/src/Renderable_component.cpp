#include "OeCore/Renderable_component.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Renderable_component);

Renderable_component::~Renderable_component() {
  _material.reset();
  _rendererData.reset();
}