#include <OeCore/Entity_render_manager.h>

#include "Scene_graph_manager.h"

#include <OeCore/Camera_component.h>
#include <OeCore/Entity_sorter.h>
#include <OeCore/ILighting_manager.h>
#include <OeCore/IMaterial_manager.h>
#include <OeCore/Light_component.h>
#include <OeCore/Mesh_utils.h>
#include <OeCore/Morph_weights_component.h>
#include <OeCore/Skinned_mesh_component.h>

#include <cinttypes>
#include <functional>
#include <optional>

using namespace oe;
using namespace internal;

using namespace std::literals;
using namespace std;

std::string Entity_render_manager::_name = "Entity_render_manager";

Renderer_animation_data g_emptyRenderableAnimationData = []() {
  auto rad = Renderer_animation_data();
  std::fill(rad.morphWeights.begin(), rad.morphWeights.end(), 0.0f);
  return rad;
}();

Entity_render_manager::Entity_render_manager(
        ITexture_manager& textureManager, IMaterial_manager& materialManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory)
    : Manager_base()
    , Manager_deviceDependent()
    , _textureManager(textureManager)
    , _materialManager(materialManager)
    , _lightingManager(lightingManager)
    , _primitiveMeshDataFactory(primitiveMeshDataFactory)
{}

void Entity_render_manager::initialize() {}

void Entity_render_manager::shutdown() {}

const std::string& Entity_render_manager::name() const
{
  return _name;
}

template<uint8_t TMax_lights>
bool addLightToRenderLightData(const Entity& lightEntity, Render_light_data_impl<TMax_lights>& renderLightData)
{
  const auto directionalLight = lightEntity.getFirstComponentOfType<Directional_light_component>();
  if (directionalLight) {
    const auto lightDirection = lightEntity.worldTransform().getUpper3x3() * math::forward;
    const auto shadowData = directionalLight->shadowData().get();

    if (shadowData != nullptr) {
      if (shadowData->shadowMap == nullptr || !shadowData->shadowMap->isArraySlice()) {
        OE_THROW(std::logic_error("Directional lights only support texture array shadow maps."));
      }

      auto shadowMapBias = directionalLight->shadowMapBias();
      return renderLightData.addDirectionalLight(
              lightDirection, directionalLight->color(), directionalLight->intensity(), *shadowData, shadowMapBias);
    }

    return renderLightData.addDirectionalLight(
            lightDirection, directionalLight->color(), directionalLight->intensity());
  }

  const auto pointLight = lightEntity.getFirstComponentOfType<Point_light_component>();
  if (pointLight) {
    return renderLightData.addPointLight(lightEntity.worldPosition(), pointLight->color(), pointLight->intensity());
  }

  const auto ambientLight = lightEntity.getFirstComponentOfType<Ambient_light_component>();
  if (ambientLight) {
    return renderLightData.addAmbientLight(ambientLight->color(), ambientLight->intensity());
  }
  return false;
}

void Entity_render_manager::createMissingVertexAttributes(
        std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& requiredAttributes,
        const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) const
{
  // TODO: Fix the normal/binormal/tangent loading code.
  auto generateNormals = false;
  auto generateTangents = false;
  auto generateBiTangents = false;

  auto layout = meshData->vertexLayout.vertexLayout();
  for (const auto& requiredAttributeElement : requiredAttributes) {
    const auto requiredAttribute = requiredAttributeElement.semantic;
    if (std::any_of(layout.begin(), layout.end(), [requiredAttribute](const auto& vae) {
          return vae.semantic == requiredAttribute;
        }))
    {
      continue;
    }

    if (std::find(vertexMorphAttributes.begin(), vertexMorphAttributes.end(), requiredAttribute) !=
        vertexMorphAttributes.end())
    {
      continue;
    }

    // We only support generation of normals, tangents, bi-tangent
    uint32_t numComponents = 3;
    if (requiredAttribute == Vertex_attribute_semantic{Vertex_attribute::Normal, 0}) {
      generateNormals = true;
    }
    else if (requiredAttribute == Vertex_attribute_semantic{Vertex_attribute::Tangent, 0}) {
      numComponents = 4;
      generateTangents = true;
    }
    else if (requiredAttribute == Vertex_attribute_semantic{Vertex_attribute::Bi_tangent, 0}) {
      generateBiTangents = true;
    }
    else {
      OE_THROW(std::logic_error("Mesh does not have required attribute: "s.append(
              Vertex_attribute_meta::vsInputName(requiredAttribute))));
    }

    // Create the missing accessor
    const auto vertexCount = meshData->getVertexCount();
    const auto elementStride = sizeof(float) * numComponents;
    meshData->vertexBufferAccessors[requiredAttribute] = make_unique<Mesh_vertex_buffer_accessor>(
            make_shared<Mesh_buffer>(elementStride * vertexCount),
            Vertex_attribute_element{
                    requiredAttribute, numComponents == 3 ? Element_type::Vector3 : Element_type::Vector4,
                    Element_component::Float},
            static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(elementStride), 0);
  }

  if (generateNormals) {
    LOG(WARNING) << "Generating missing normals";
    _primitiveMeshDataFactory.generateNormals(
            *meshData->indexBufferAccessor, *meshData->vertexBufferAccessors[{Vertex_attribute::Position, 0}],
            *meshData->vertexBufferAccessors[{Vertex_attribute::Normal, 0}]);
  }

  if (generateTangents || generateBiTangents) {
    LOG(WARNING) << "Generating missing Tangents and/or Bi-tangents";
    _primitiveMeshDataFactory.generateTangents(meshData);
  }
}

void Entity_render_manager::renderEntity(
        Renderable_component& renderableComponent, const Camera_data& cameraData,
        const Light_provider::Callback_type& lightDataProvider, const Render_pass_blend_mode blendMode)
{
  if (!renderableComponent.visible()) {
    return;
  }

  const auto& entity = renderableComponent.getEntity();

  try {
    const auto material = renderableComponent.material();
    if (!material) {
      OE_THROW(std::runtime_error("Missing material on entity"));
    }

    const auto meshDataComponent = entity.getFirstComponentOfType<Mesh_data_component>();
    if (meshDataComponent == nullptr || meshDataComponent->meshData() == nullptr) {
      // There is no mesh data (it may still be loading!), we can't render.
      return;
    }

    const auto& meshData = meshDataComponent->meshData();

    auto rendererData = renderableComponent.rendererData().lock();
    if (rendererData == nullptr) {
      LOG(INFO) << "Creating renderer data for entity " << entity.getName() << " (ID " << entity.getId() << ")";

      // Note we get the flags for the case where all features are enabled, to make sure we load all
      // the data streams.
      const auto flags = material->configFlags(
              Renderer_features_enabled(), blendMode, meshDataComponent->meshData()->vertexLayout);
      const auto vertexInputs = material->vertexInputs(flags);
      const auto vertexSettings = material->vertexShaderSettings(flags);

      rendererData = createRendererData(meshData, vertexInputs, vertexSettings.morphAttributes);
      renderableComponent.setRendererData(std::weak_ptr(rendererData));
    }

    auto materialContext = renderableComponent.materialContext().lock();
    // This may be null if the device was reset since the last render.
    if (materialContext == nullptr) {
      renderableComponent.setMaterialContext(_materialManager.createMaterialContext());

      materialContext = renderableComponent.materialContext().lock();

      if (materialContext == nullptr) {
        OE_THROW(std::runtime_error("Failed to create a material context"));
      }
    }

    const auto lightMode = material->lightMode();
    Render_light_data* renderLightData = nullptr;

    // TODO: FIXME: I think this is really broken. We shouldn't be clearing the render light data per entity; but instead per frame.
    if (Material_light_mode::Lit == lightMode) {
      auto renderLightDataLit = _lightingManager.getRenderLightDataLit();
      auto maxLights = renderLightDataLit->getMaxLights();

      renderLightData = renderLightDataLit;

      // Ask the caller what lights are affecting this entity.
      _renderLights.clear();
      renderLightDataLit->clear();
      lightDataProvider(entity.boundSphere(), _renderLights, maxLights);
      if (_renderLights.size() > maxLights) {
        OE_THROW(std::logic_error("Light_provider::Callback_type added too many lights to entity"));
      }

      for (auto& lightEntity : _renderLights) {
        if (!addLightToRenderLightData(*lightEntity, *renderLightDataLit)) {
          OE_THROW(std::logic_error("Failed to add light to light data"));
        }
      }

      _materialManager.updateLightBuffers();
    }
    else {
      renderLightData = _lightingManager.getRenderLightDataUnlit();
    }

    // Morphed animation?
    auto* const morphWeightsComponent = entity.getFirstComponentOfType<Morph_weights_component>();
    if (morphWeightsComponent != nullptr) {
      std::transform(
              morphWeightsComponent->morphWeights().begin(), morphWeightsComponent->morphWeights().end(),
              _rendererAnimationData.morphWeights.begin(), [](double v) { return static_cast<float>(v); });
      std::fill(
              _rendererAnimationData.morphWeights.begin() + morphWeightsComponent->morphWeights().size(),
              _rendererAnimationData.morphWeights.end(), 0.0f);
    }

    // Skinned mesh?
    auto* const skinnedMeshComponent = entity.getFirstComponentOfType<Skinned_mesh_component>();
    // Matrix const* worldTransform;
    const SSE::Matrix4* worldTransform;
    const auto skinningEnabled = _materialManager.rendererFeatureEnabled().skinnedAnimation;
    if (skinningEnabled && skinnedMeshComponent != nullptr) {

      // Don't need the transform root here, just read the world transforms.
      const auto& joints = skinnedMeshComponent->joints();
      const auto& inverseBindMatrices = skinnedMeshComponent->inverseBindMatrices();
      if (joints.size() != inverseBindMatrices.size())
        OE_THROW(std::runtime_error("Size of joints and inverse bone transform arrays must match."));

      if (inverseBindMatrices.size() > _rendererAnimationData.boneTransformConstants.size()) {
        OE_THROW(std::runtime_error(
                "Maximum number of bone transforms exceeded: " + std::to_string(inverseBindMatrices.size()) + " > " +
                std::to_string(_rendererAnimationData.boneTransformConstants.size())));
      }

      if (skinnedMeshComponent->skeletonTransformRoot())
        worldTransform = &skinnedMeshComponent->skeletonTransformRoot()->worldTransform();
      else
        worldTransform = &entity.worldTransform();

      auto invWorld = SSE::inverse(*worldTransform);
      for (size_t i = 0; i < joints.size(); ++i) {
        auto* const joint = joints[i];
        const auto& jointWorldTransform = joint->worldTransform();
        auto jointToRoot = invWorld * jointWorldTransform;
        const auto inverseBoneTransform = inverseBindMatrices[i];

        _rendererAnimationData.boneTransformConstants[i] = jointToRoot * inverseBoneTransform;
      }
      _rendererAnimationData.numBoneTransforms = static_cast<uint32_t>(joints.size());
    }
    else {
      _rendererAnimationData.numBoneTransforms = 0;
      worldTransform = &entity.worldTransform();
    }

    drawRendererData(
            cameraData, *worldTransform, *rendererData, blendMode, *renderLightData, material, meshData->vertexLayout,
            *materialContext, _rendererAnimationData, renderableComponent.wireframe());
  }
  catch (std::runtime_error& e) {
    renderableComponent.setVisible(false);
    LOG(WARNING) << "Failed to render mesh on entity " << entity.getName() << " (ID " << entity.getId()
                 << "): " << e.what();
  }
}

void Entity_render_manager::renderRenderable(
        Renderable& renderable, const SSE::Matrix4& worldMatrix, float radius, const Camera_data& cameraData,
        const Light_provider::Callback_type& lightDataProvider, Render_pass_blend_mode blendMode, bool wireFrame)
{
  auto material = renderable.material;

  auto rendererData = renderable.rendererData.lock();
  if (rendererData == nullptr) {
    if (renderable.meshData == nullptr) {
      // There is no mesh data (it may still be loading!), we can't render.
      return;
    }

    LOG(INFO) << "Creating renderer data for renderable";

    // Note we get the flags for the case where all features are enabled, to make sure we load all
    // the data streams.
    const auto flags = material->configFlags(Renderer_features_enabled(), blendMode, renderable.meshData->vertexLayout);
    const auto vertexInputs = material->vertexInputs(flags);
    const auto vertexSettings = material->vertexShaderSettings(flags);

    rendererData = createRendererData(renderable.meshData, vertexInputs, vertexSettings.morphAttributes);
    renderable.rendererData = std::weak_ptr(rendererData);
  }

  auto materialContext = renderable.materialContext.lock();
  // This may be null if the device was reset since the last render.
  if (materialContext == nullptr) {
    renderable.materialContext = _materialManager.createMaterialContext();
    materialContext = renderable.materialContext.lock();

    if (materialContext == nullptr) {
      OE_THROW(std::runtime_error("Failed to create a material context"));
    }
  }

  const auto lightMode = material->lightMode();
  Render_light_data* renderLightData;
  if (Material_light_mode::Lit == lightMode) {
    auto* const renderLightDataLit = _lightingManager.getRenderLightDataLit();
    renderLightData = renderLightDataLit;

    // Ask the caller what lights are affecting this entity.
    _renderLights.clear();
    renderLightDataLit->clear();
    BoundingSphere lightTarget;
    lightTarget.center = worldMatrix.getTranslation();
    lightTarget.radius = radius;
    lightDataProvider(lightTarget, _renderLights, renderLightDataLit->maxLights());
    if (_renderLights.size() > renderLightDataLit->maxLights()) {
      OE_THROW(std::logic_error("Light_provider::Callback_type added too many lights to entity"));
    }

    for (auto& lightEntity : _renderLights) {
      if (!addLightToRenderLightData(*lightEntity, *renderLightDataLit)) {
        OE_THROW(std::logic_error("Failed to add light to light data"));
      }
    }

    _materialManager.updateLightBuffers();
  }
  else {
    renderLightData = _lightingManager.getRenderLightDataUnlit();
  }

  auto* rendererAnimationData = renderable.rendererAnimationData.get();
  if (rendererAnimationData == nullptr) rendererAnimationData = &g_emptyRenderableAnimationData;

  drawRendererData(
          cameraData, worldMatrix, *rendererData, blendMode, *renderLightData, material,
          renderable.meshData->vertexLayout, *materialContext, *rendererAnimationData, wireFrame);
}

Renderable Entity_render_manager::createScreenSpaceQuad(std::shared_ptr<Material> material)
{
  auto renderable = Renderable();
  if (renderable.meshData == nullptr)
    renderable.meshData = _primitiveMeshDataFactory.createQuad(2.0f, 2.0f, {-1.f, -1.f, 0.f});

  if (renderable.material == nullptr) renderable.material = material;

  auto rendererData = renderable.rendererData.lock();
  if (rendererData == nullptr) {
    // Note we get the flags for the case where all features are enabled, to make sure we load all
    // the data streams.
    const auto flags = material->configFlags(
            Renderer_features_enabled(), Render_pass_blend_mode::Opaque, renderable.meshData->vertexLayout);
    std::vector<Vertex_attribute> vertexAttributes;
    const auto vertexInputs = renderable.material->vertexInputs(flags);
    const auto vertexSettings = material->vertexShaderSettings(flags);
    rendererData = createRendererData(renderable.meshData, vertexInputs, vertexSettings.morphAttributes);
    renderable.rendererData = std::weak_ptr(rendererData);
  }

  return renderable;
}

void Entity_render_manager::clearRenderStats()
{
  _renderStats = {};
}

oe::Mesh_vertex_buffer_accessor* Entity_render_manager::findAccessorForSemantic(
        std::shared_ptr<Mesh_data>& meshData, const gsl::span<const Vertex_attribute_semantic>& attributes,
        const oe::Vertex_attribute_semantic& vertexAttrSemantic) const
{
  Mesh_vertex_buffer_accessor* meshAccessor;
  auto vbAccessorPos = meshData->vertexBufferAccessors.find(vertexAttrSemantic);
  if (vbAccessorPos != meshData->vertexBufferAccessors.end()) {
    meshAccessor = vbAccessorPos->second.get();
  } else {
    // Since there is no guarantee that the order is the same in the requested attributes array,
    // and the mesh data, we need to do a search to find out the morph target index of this
    // attribute/semantic.
    auto vertexMorphAttributesIdx = -1;
    auto morphTargetIdx = -1;
    for (size_t idx = 0; idx < attributes.size(); ++idx) {
      const auto& vertexMorphAttribute = attributes[idx];
      if (vertexMorphAttribute.attribute == vertexAttrSemantic.attribute) {
        ++morphTargetIdx;

        if (vertexMorphAttribute.semanticIndex == vertexAttrSemantic.semanticIndex) {
          vertexMorphAttributesIdx = static_cast<int>(idx);
          break;
        }
      }
    }
    if (vertexMorphAttributesIdx == -1) {
      OE_THROW(std::runtime_error("Could not find morph attribute in vertexMorphAttributes"));
    }

    const size_t morphTargetLayoutSize = meshData->vertexLayout.morphTargetLayout().size();
    // What position in the mesh morph layout is this type? This will correspond with its
    // position in the buffer accessors array
    auto morphLayoutOffset = -1;
    for (size_t idx = 0; idx < morphTargetLayoutSize; ++idx) {
      if (meshData->vertexLayout.morphTargetLayout()[idx].attribute == vertexAttrSemantic.attribute) {
        morphLayoutOffset = static_cast<int>(idx);
        break;
      }
    }
    if (morphLayoutOffset == -1) {
      OE_THROW(
              std::runtime_error("Could not find morph attribute in mesh morph target layout"));
    }

    if (meshData->attributeMorphBufferAccessors.size() >=
        static_cast<size_t>(morphLayoutOffset)) {
      std::string msg = string_format(
              "CreateRendererData: Failed to read morph target "
              "%" PRIi32 " for vertex attribute: %s",
              morphTargetIdx,
              Vertex_attribute_meta::vsInputName(vertexAttrSemantic).c_str());
      OE_THROW(std::runtime_error(msg));
    }
    if (meshData->attributeMorphBufferAccessors.at(morphTargetIdx).size() >=
        static_cast<size_t>(morphTargetIdx)) {
      OE_THROW(std::runtime_error(string_format(
              "CreateRendererData: Failed to read morph target "
              "%" PRIi32 " layout offset %" PRIi32 "for vertex attribute: %s",
              morphTargetIdx,
              morphLayoutOffset,
              Vertex_attribute_meta::vsInputName(vertexAttrSemantic).c_str())));
    }

    meshAccessor =
            meshData->attributeMorphBufferAccessors[morphTargetIdx][morphLayoutOffset].get();
  }
  return meshAccessor;
}