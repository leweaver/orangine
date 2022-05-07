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
    // State that the compiled shader relied on
    size_t materialHash = 0;
    size_t meshHash = 0;
    size_t rendererFeaturesHash = 0;
    Depth_stencil_config depthStencilConfig;
    std::vector<Vertex_attribute_element> vsInputs;
    std::set<std::string> flags;
    Mesh_index_type meshIndexType;

    // Used for debug identification
    std::string name;
  };
  struct Pipeline_state_inputs {
    // Options that don't need a recompile, but will need to modify pipeline state
    bool wireframe;
  };

  Material_context() = default;

  virtual ~Material_context() = default;

  virtual void releaseResources() = 0;

  bool compilerInputsValid = false;
  Material_compiler_inputs compilerInputs;
  Pipeline_state_inputs pipelineStateInputs;
};
} // namespace oe
