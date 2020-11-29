#pragma once

#include "Renderer_types.h"

#include <set>

namespace oe {
/**
 * Material context is a where a compiled Material resources are stored for a
 * specific mesh. This is necessary as multiple renderables (with differing layouts)
 * can point to a single Material instance.
 *
 * Don't modify anything in here directly, unless you are Material_manager::bind
 */
class Material_context {
 public:
  struct Compiled_material {
    // State that the compiled state relied on
    size_t materialHash;
    size_t meshHash;
    size_t rendererFeaturesHash;
    Render_pass_blend_mode blendMode;
    std::vector<Vertex_attribute_element> vsInputs;
    std::set<std::string> flags;
  };

  Material_context() = default;

  virtual ~Material_context() {}

  virtual void reset() = 0;

  std::shared_ptr<Compiled_material> compiledMaterial;
};
} // namespace oe
