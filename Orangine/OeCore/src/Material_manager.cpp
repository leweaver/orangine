#include "Material_manager.h"

#include "OeCore/Material_context.h"
#include "OeCore/Mesh_utils.h"

#include <locale>

using namespace oe;
using namespace DirectX;
using namespace std::literals;

const auto g_max_material_index = UINT8_MAX;
const std::string g_flag_disable_optimisation = "disableOptimizations";

std::string Material_manager::_name = "Material_manager";

Material_manager::Material_manager(IAsset_manager& assetManager)
    : Manager_base()
      , Manager_tickable()
      , Manager_deviceDependent()
      ,_boundBlendMode(Render_pass_blend_mode::Opaque)
    , _assetManager(assetManager)
{}

void Material_manager::setShaderPath(const std::string& path) { _shaderPath = path; }

const std::string& Material_manager::shaderPath() const { return _shaderPath; }

void Material_manager::initialize() {
  _shaderPath = _assetManager.makeAbsoluteAssetPath("OeCore/shaders");
  setRendererFeaturesEnabled(Renderer_features_enabled());
}

const std::string& Material_manager::name() const { return _name; }

void Material_manager::tick() {
  if (_boundMaterial) {
    OE_THROW(
        std::logic_error("Material is still bound after rendering complete! Did you forget to call "
                         "IMaterial_manager::unbind() ?"));
  }
}

void Material_manager::bind(
    Material_context& materialContext,
    std::shared_ptr<const Material> material,
    const Mesh_vertex_layout& meshVertexLayout,
    const Render_light_data* renderLightData,
    Render_pass_blend_mode blendMode,
    bool enablePixelShader) {
  assert(!_boundMaterial);

  const auto materialHash = material->ensureCompilerPropertiesHash();
  auto& compiledMaterial = materialContext.compilerInputs;
  auto rebuildConfig = false;
  if (!materialContext.compilerInputsValid) {
    rebuildConfig = true;
  } else {
    if (compiledMaterial.blendMode != blendMode) {
      LOG(WARNING) << "Rendering material with a different blendMode than last time. This will be "
                      "a big performance hit.";
      rebuildConfig = true;
    } else if (compiledMaterial.meshHash != meshVertexLayout.propertiesHash()) {
      LOG(WARNING) << "Rendering material with a different morphTargetCount than last time. This "
                      "will be a big performance hit.";
      rebuildConfig = true;
    } else if (compiledMaterial.materialHash != materialHash) {
      LOG(DEBUG) << "Material hash changed.";
      rebuildConfig = true;
    } else if (compiledMaterial.rendererFeaturesHash != _rendererFeaturesHash) {
      LOG(DEBUG) << "Renderer features hash changed.";
      rebuildConfig = true;
    }
  }

  if (rebuildConfig) {
    auto flags = material->configFlags(_rendererFeatures, blendMode, meshVertexLayout);

    // Add flag for shader optimisation, to determine if we need to recompile
    if (!_rendererFeatures.enableShaderOptimization) {
      flags.insert(g_flag_disable_optimisation);
    }

    LOG(DEBUG) << "Material flags: " << nlohmann::json(flags).dump(2);

    auto requiresRecompile = true;
    // Skip recompile if the flags are actually the same.
    if (flags.size() == compiledMaterial.flags.size() &&
        std::equal(flags.begin(), flags.end(), compiledMaterial.flags.begin())) {
      requiresRecompile = false;
    }
    compiledMaterial.rendererFeaturesHash = _rendererFeaturesHash;

    if (requiresRecompile) {
      LOG(INFO) << "Recompiling shaders for material";

      // TODO: Look in a cache for a compiled material that matches the hash
      materialContext.compilerInputs = compiledMaterial;
      materialContext.compilerInputsValid = true;

      compiledMaterial.materialHash = materialHash;
      compiledMaterial.meshHash = meshVertexLayout.propertiesHash();

      try {
        compiledMaterial.blendMode = blendMode;
        compiledMaterial.flags = std::move(flags);

        compiledMaterial.vsInputs = material->vertexInputs(compiledMaterial.flags);

        createVertexShader(_rendererFeatures.enableShaderOptimization, *material, materialContext);
        createPixelShader(_rendererFeatures.enableShaderOptimization, *material, materialContext);

        createMaterialConstants(*material);

        // Make sure that the shader resource views and SamplerStates vectors are empty.
        materialContext.reset();

      } catch (std::exception& ex) {
        materialContext.compilerInputsValid = false;
        OE_THROW(std::runtime_error(
            "Failed to create resources in Material_manager::bind. "s + ex.what()));
      }
    }

    const auto shaderResources =
        material->shaderResources(compiledMaterial.flags, *renderLightData);

    loadShaderResourcesToContext(shaderResources, materialContext);
  }

  // If we get to this point, we have a valid and compiled shader. Now bind it to the device.
  bindLightDataToDevice(renderLightData);
  bindMaterialContextToDevice(materialContext, enablePixelShader);

  _boundMaterial = material;
  _boundBlendMode = blendMode;
}

void Material_manager::unbind() { _boundMaterial.reset(); }

void Material_manager::setRendererFeaturesEnabled(
    const Renderer_features_enabled& renderer_feature_enabled) {
  _rendererFeatures = renderer_feature_enabled;
  _rendererFeaturesHash = _rendererFeatures.hash();
}

const Renderer_features_enabled& Material_manager::rendererFeatureEnabled() const {
  return _rendererFeatures;
}
