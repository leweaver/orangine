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
  struct Material_compiler_inputs {
    // State that the compiled state relied on
    size_t materialHash = 0;
    size_t meshHash = 0;
    size_t rendererFeaturesHash = 0;
    Render_pass_blend_mode blendMode = Render_pass_blend_mode::Opaque;
    std::vector<Vertex_attribute_element> vsInputs;
    std::set<std::string> flags;
  };

  Material_context() = default;

  virtual ~Material_context() = default;

  virtual void reset() = 0;

  bool compilerInputsValid = false;
  Material_compiler_inputs compilerInputs;
};
} // namespace oe
