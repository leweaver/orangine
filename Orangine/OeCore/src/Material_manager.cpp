#include <OeCore/Material_manager.h>

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

void Material_manager::initialize()
{
  _shaderPath = _assetManager.makeAbsoluteAssetPath("OeCore/shaders");
  setRendererFeaturesEnabled(Renderer_features_enabled());
}

void Material_manager::tick()
{
  if (_boundMaterial) {
    OE_THROW(std::logic_error("Material is still bound after rendering complete! Did you forget to call "
                              "IMaterial_manager::unbind() ?"));
  }
}

void Material_manager::updateMaterialContext(
        Material_context_handle materialContextHandle, std::shared_ptr<const Material> material,
        const Mesh_vertex_layout& meshVertexLayout, const Mesh_gpu_data& meshGpuData,
        const Render_light_data* renderLightData, const Depth_stencil_config& depthStencilConfig,
        bool enablePixelShader, bool wireframe)
{
  LOG(DEBUG) << "Binding material " << material->materialType();
  OE_CHECK(!_boundMaterial);

  const auto materialHash = material->ensureCompilerPropertiesHash();
  Material_state_identifier stateIdentifier{};
  bool rebuildConfig = !getMaterialStateIdentifier(materialContextHandle, stateIdentifier);
  if (!rebuildConfig) {
    if (stateIdentifier.depthStencilModeHash != depthStencilConfig.getModeHash()) {
      LOG(DEBUG) << "Rendering material with a different depth stencil config than last frame. This "
                    "will be a big performance hit.\n"
                 << "New config: " << depthStencilConfig;
      rebuildConfig = true;
    }
    else if (stateIdentifier.meshHash != meshVertexLayout.propertiesHash()) {
      LOG(WARNING) << "Rendering material with a different mesh vertex layout than last frame. This "
                      "will be a big performance hit.";
      rebuildConfig = true;
    }
    else if (stateIdentifier.materialHash != materialHash) {
      LOG(DEBUG) << "Material hash changed.";
      rebuildConfig = true;
    }
    else if (stateIdentifier.rendererFeaturesHash != _rendererFeaturesHash) {
      LOG(DEBUG) << "Renderer features hash changed.";
      rebuildConfig = true;
    }
  }

  if (rebuildConfig) {
    auto flags = material->configFlags(_rendererFeatures, depthStencilConfig.getBlendMode(), meshVertexLayout);

    // Add flag for shader optimisation, to determine if we need to recompile
    if (!_rendererFeatures.enableShaderOptimization) {
      flags.insert(g_flag_disable_optimisation);
    }

    LOG(DEBUG) << "Material flags: " << nlohmann::json(flags).dump(2);

    // Are the textures on the material loaded and ready to use?
    const Material::Shader_resources shaderResources = material->shaderResources(flags, *renderLightData);
    bool waitingOnTextureLoad = false;
    for (const auto& textureResource : shaderResources.textures) {
      if (!textureResource.texture->isValid()) {
        LOG(DEBUG) << "Texture is not loaded yet: " << textureResource.texture->getName();
        queueTextureLoad(*textureResource.texture);
        waitingOnTextureLoad = true;
      }
    }
    if (waitingOnTextureLoad) {
      setDataReady(materialContextHandle, false);
      return;
    }

    bool requiresRecompile = true;
    /*
    // Skip recompile if the flags are actually the same.
    if (flags.size() == compiledMaterial.flags.size() &&
        std::equal(flags.begin(), flags.end(), compiledMaterial.flags.begin()))
    {
      requiresRecompile = false;
    }
     */
    stateIdentifier.rendererFeaturesHash = _rendererFeaturesHash;

    if (requiresRecompile) {

#if !defined(NDEBUG)
      bool enableOptimizations = false;
#else
      bool enableOptimizations = true;
#endif

      Material_compiler_inputs compilerInputs{depthStencilConfig,  material->vertexInputs(flags),
                                              std::move(flags),    meshVertexLayout.getMeshIndexType(),
                                              enableOptimizations, material->materialType()};
      LOG(INFO) << "Recompiling shaders for material";

      // TODO: Look in a cache for a compiled material that matches the hash
      stateIdentifier.depthStencilModeHash = depthStencilConfig.getModeHash();
      stateIdentifier.materialHash = materialHash;
      stateIdentifier.meshHash = meshVertexLayout.propertiesHash();

      try {
        loadMaterialToContext(materialContextHandle, *material, stateIdentifier, compilerInputs);
      }
      catch (std::exception& ex) {
        OE_THROW(std::runtime_error(
                "Failed to create resources in Material_manager::updateMaterialContext. "s + ex.what()));
      }
    }

    loadResourcesToContext(materialContextHandle, shaderResources, meshVertexLayout.vertexLayout());
  }

  bool updatePipelineState = rebuildConfig || stateIdentifier.wireframeEnabled != wireframe;
  if (updatePipelineState) {
    Pipeline_state_inputs pipelineStateInputs{wireframe};
    loadPipelineStateToContext(materialContextHandle, pipelineStateInputs);
  }
  setDataReady(materialContextHandle, true);
}

void Material_manager::bind(Material_context_handle materialContextHandle)
{
  OE_CHECK(!_boundMaterial);

  bindMaterialContextToDevice(materialContextHandle);

  _boundMaterial = true;
}


void Material_manager::unbind() { _boundMaterial = false; }

void Material_manager::setRendererFeaturesEnabled(const Renderer_features_enabled& renderer_feature_enabled)
{
  _rendererFeatures = renderer_feature_enabled;
  _rendererFeaturesHash = _rendererFeatures.hash();
}

void Material_manager::debugLogSettings(const char* prefix, const Material::Shader_compile_settings& settings) const
{
  if (g3::logLevel(DEBUG)) {
    LOG(DEBUG) << prefix << " compile settings: "
               << nlohmann::json({{"defines", settings.defines},
                                  {"includes", settings.includes},
                                  {"filename", settings.filename},
                                  {"entryPoint", settings.entryPoint}})
                          .dump(2);
  }
}

const Renderer_features_enabled& Material_manager::rendererFeatureEnabled() const { return _rendererFeatures; }
