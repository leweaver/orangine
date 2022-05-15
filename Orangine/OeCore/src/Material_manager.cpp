#include <OeCore/Material_manager.h>

#include <OeCore/Material_context.h>
#include <OeCore/Mesh_utils.h>

#include <locale>

using namespace oe;
using namespace DirectX;
using namespace std::literals;

const auto g_max_material_index = UINT8_MAX;
const std::string g_flag_disable_optimisation = "disableOptimizations";

Material_manager::Material_manager(IAsset_manager& assetManager)
    : Manager_base()
      , Manager_tickable()
      , Manager_deviceDependent()
    , _assetManager(assetManager)
{}

void Material_manager::setShaderPath(const std::string& path) { _shaderPath = path; }

const std::string& Material_manager::shaderPath() const { return _shaderPath; }

void Material_manager::initialize() {
  _shaderPath = _assetManager.makeAbsoluteAssetPath("OeCore/shaders");
  setRendererFeaturesEnabled(Renderer_features_enabled());
}

void Material_manager::tick() {
  if (_boundMaterial) {
    OE_THROW(
        std::logic_error("Material is still bound after rendering complete! Did you forget to call "
                         "IMaterial_manager::unbind() ?"));
  }
}

void Material_manager::updateMaterialContext(
    Material_context& materialContext,
    std::shared_ptr<const Material> material,
    const Mesh_vertex_layout& meshVertexLayout,
    const Mesh_gpu_data& meshGpuData,
    const Render_light_data* renderLightData,
    const Depth_stencil_config& depthStencilConfig,
    bool enablePixelShader, bool wireframe) {
  LOG(INFO) << "Binding material " << material->materialType();
  OE_CHECK(!_boundMaterial);

  const auto materialHash = material->ensureCompilerPropertiesHash();
  auto& compiledMaterial = materialContext.compilerInputs;
  auto rebuildConfig = false;
  if (!materialContext.compilerInputsValid) {
    rebuildConfig = true;
  } else {
    if (materialContext.compilerInputs.depthStencilConfig.getModeHash() != depthStencilConfig.getModeHash()) {
      LOG(DEBUG) << "Rendering material with a different depth stencil config than last frame. This "
                    "will be a big performance hit.\n"
                 << "Previous config: " << materialContext.compilerInputs.depthStencilConfig << "\n"
              << "New config: " << depthStencilConfig;
      rebuildConfig = true;
    } else if (compiledMaterial.meshHash != meshVertexLayout.propertiesHash()) {
      LOG(WARNING) << "Rendering material with a different mesh vertex layout than last frame. This "
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

  bool updatePipelineState = rebuildConfig || materialContext.pipelineStateInputs.wireframe != wireframe;

  if (rebuildConfig) {
    auto flags = material->configFlags(_rendererFeatures, depthStencilConfig.getBlendMode(), meshVertexLayout);

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

      // Make sure that the shader resource views and SamplerStates vectors are empty.
      materialContext.releaseResources();

      // TODO: Look in a cache for a compiled material that matches the hash
      compiledMaterial.materialHash = materialHash;
      compiledMaterial.meshHash = meshVertexLayout.propertiesHash();
      compiledMaterial.meshIndexType = meshVertexLayout.getMeshIndexType();

      try {
        compiledMaterial.depthStencilConfig = depthStencilConfig;
        compiledMaterial.flags = std::move(flags);

        compiledMaterial.vsInputs = material->vertexInputs(compiledMaterial.flags);
        compiledMaterial.name = material->materialType();

        materialContext.compilerInputs = compiledMaterial;
        materialContext.compilerInputsValid = true;

        loadMaterialToContext(*material, materialContext, _rendererFeatures.enableShaderOptimization);

      } catch (std::exception& ex) {
        materialContext.compilerInputsValid = false;
        OE_THROW(std::runtime_error(
            "Failed to create resources in Material_manager::updateMaterialContext. "s + ex.what()));
      }
    }

    const auto shaderResources =
        material->shaderResources(compiledMaterial.flags, *renderLightData);

    loadResourcesToContext(shaderResources, meshVertexLayout.vertexLayout(), materialContext);
  }

  if (updatePipelineState) {
    materialContext.pipelineStateInputs.wireframe = wireframe;
    loadPipelineStateToContext(materialContext);
  }
}

void Material_manager::bind(Material_context& materialContext)
{
  OE_CHECK(!_boundMaterial);

  bindMaterialContextToDevice(materialContext);

  _boundMaterial = true;
}


void Material_manager::unbind() {
  LOG(INFO) << "Unbinding material";
  _boundMaterial = false;
}

void Material_manager::setRendererFeaturesEnabled(
    const Renderer_features_enabled& renderer_feature_enabled) {
  _rendererFeatures = renderer_feature_enabled;
  _rendererFeaturesHash = _rendererFeatures.hash();
}

void Material_manager::debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings) const {
  if (g3::logLevel(DEBUG)) {
    LOG(DEBUG) << prefix << " compile settings: "
               << nlohmann::json({{"defines", settings.defines},
                                  {"includes", settings.includes},
                                  {"filename", settings.filename},
                                  {"entryPoint", settings.entryPoint}})
                          .dump(2);
  }
}

const Renderer_features_enabled& Material_manager::rendererFeatureEnabled() const {
  return _rendererFeatures;
}
