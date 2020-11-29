#include "pch.h"

#include "OeCore/JsonUtils.h"
#include "OeCore/Material_repository.h"
#include "OeCore/Mesh_vertex_layout.h"
#include "OeCore/PBR_material.h"
#include "OeCore/Render_pass_config.h"

using namespace oe;
using namespace std::literals;

const std::string g_material_type = "PBR_material";
const std::string g_json_baseColor = "baseColor";
const std::string g_json_metallic = "metallic";
const std::string g_json_roughness = "roughness";
const std::string g_json_emissive = "emissive";
const std::string g_json_alphaCutoff = "alpha_cutoff";
const std::string g_json_baseColorTexture = "base_color_texture";
const std::string g_json_metallicRoughnessTexture = "metallic_roughness_texture";
const std::string g_json_normalTexture = "normal_texture";
const std::string g_json_occlusionTexture = "occlusion_texture";
const std::string g_json_emissiveTexture = "emissive_texture";

const std::string g_flag_enableDeferred = "enable_deferred";
const std::string g_flag_morphTargetCount_prefix = "morph_target_count_";
// Position in the layout of each morph attribute
const std::string g_flag_morphTargetLayout_Position_prefix = "morph_target_vertex_position_";
const std::string g_flag_morphTargetLayout_Normal_prefix = "morph_target_vertex_normal_";
const std::string g_flag_morphTargetLayout_Tangent_prefix = "morph_target_vertex_tangent_";
const std::string g_flag_skinned = "skinned";
const std::string g_flag_skinned_joints_uint16 = "joints_uint16";
const std::string g_flag_skinned_joints_uint32 = "joints_uint32";
const std::string g_flag_skinned_joints_sint16 = "joints_sint16";
const std::string g_flag_skinned_joints_sint32 = "joints_sint32";

PBR_material::PBR_material()
    : Base_type(static_cast<uint8_t>(Material_type_index::PBR)), _baseColor(Colors::White),
      _metallic(1.0), _roughness(1.0), _emissive(Colors::Black), _alphaCutoff(0.5),
      _boundTextureCount(0)
{
  std::fill(_textures.begin(), _textures.end(), nullptr);
  std::fill(_shaderResourceViews.begin(), _shaderResourceViews.end(), nullptr);
  std::fill(_samplerStates.begin(), _samplerStates.end(), nullptr);
}

const std::string& PBR_material::materialType() const { return g_material_type; }

nlohmann::json PBR_material::serialize(bool compilerPropertiesOnly) const
{
  auto j = Base_type::serialize(compilerPropertiesOnly);

  if (!compilerPropertiesOnly) {
    j[g_json_baseColor] = _baseColor;
    j[g_json_metallic] = _metallic;
    j[g_json_roughness] = _roughness;
    j[g_json_emissive] = _emissive;
    j[g_json_alphaCutoff] = _alphaCutoff;
    j[g_json_baseColorTexture] = serializeTexture(compilerPropertiesOnly, _textures[BaseColor]);
    j[g_json_metallicRoughnessTexture] =
        serializeTexture(compilerPropertiesOnly, _textures[MetallicRoughness]);
    j[g_json_normalTexture] = serializeTexture(compilerPropertiesOnly, _textures[Normal]);
    j[g_json_occlusionTexture] = serializeTexture(compilerPropertiesOnly, _textures[Occlusion]);
    j[g_json_emissiveTexture] = serializeTexture(compilerPropertiesOnly, _textures[Emissive]);
  }

  return j;
}

std::set<std::string> PBR_material::configFlags(const Renderer_features_enabled& rendererFeatures,
                                                Render_pass_blend_mode blendMode,
                                                const Mesh_vertex_layout& meshVertexLayout) const
{
  auto flags = Base_type::configFlags(rendererFeatures, blendMode, meshVertexLayout);

  // TODO: This is a hacky way of finding this information out.
  if (blendMode == Render_pass_blend_mode::Opaque) {
    flags.insert(g_flag_enableDeferred);
  }

  const auto vertexLayout = meshVertexLayout.vertexLayout();
  const auto hasJoints = std::any_of(vertexLayout.begin(), vertexLayout.end(), [](const auto& vae) {
    return vae.semantic == Vertex_attribute_semantic{Vertex_attribute::Joints, 0};
  });
  const auto hasWeights =
      std::any_of(vertexLayout.begin(), vertexLayout.end(), [](const auto& vae) {
        return vae.semantic == Vertex_attribute_semantic{Vertex_attribute::Weights, 0};
      });

  if (rendererFeatures.skinnedAnimation && hasJoints && hasWeights) {
    flags.insert(g_flag_skinned);

    for (const auto mve : meshVertexLayout.vertexLayout()) {
      if (mve.semantic == Vertex_attribute_semantic{Vertex_attribute::Joints, 0}) {
        if (mve.component == Element_component::Unsigned_Short)
          flags.insert(g_flag_skinned_joints_uint16);
        else if (mve.component == Element_component::Unsigned_Int)
          flags.insert(g_flag_skinned_joints_uint32);
        else if (mve.component == Element_component::Signed_Short)
          flags.insert(g_flag_skinned_joints_sint16);
        else if (mve.component == Element_component::Signed_Int)
          flags.insert(g_flag_skinned_joints_sint32);
        else
          OE_THROW(std::runtime_error("Material does not support joints component: " +
                                   elementComponentToString(mve.component)));
      }
    }
  }

  if (rendererFeatures.vertexMorph && meshVertexLayout.morphTargetCount()) {
    flags.insert(g_flag_morphTargetCount_prefix +
                 std::to_string(meshVertexLayout.morphTargetCount()));

    const auto& morphTargetLayout = meshVertexLayout.morphTargetLayout();
    for (size_t i = 0; i < morphTargetLayout.size(); ++i) {
      if (morphTargetLayout[i] == Vertex_attribute_semantic{Vertex_attribute::Position, 0}) {
        flags.insert(g_flag_morphTargetLayout_Position_prefix + std::to_string(i));
      }
      else if (morphTargetLayout[i] == Vertex_attribute_semantic{Vertex_attribute::Normal, 0}) {
        flags.insert(g_flag_morphTargetLayout_Normal_prefix + std::to_string(i));
      }
      else if (morphTargetLayout[i] == Vertex_attribute_semantic{Vertex_attribute::Tangent, 0}) {
        flags.insert(g_flag_morphTargetLayout_Tangent_prefix + std::to_string(i));
      }
    }
  }

  return flags;
}

void PBR_material::decodeMorphTargetConfig(const std::set<std::string>& flags, uint8_t& targetCount,
                                           int8_t& positionPosition, int8_t& normalPosition,
                                           int8_t& tangentPosition)
{
  targetCount = 0;
  positionPosition = -1;
  normalPosition = -1;
  tangentPosition = -1;
  for (const auto& flag : flags) {
    uint16_t convert;
    if (str_starts(flag, g_flag_morphTargetCount_prefix)) {
      std::stringstream ss(flag.substr(g_flag_morphTargetCount_prefix.size()));
      ss >> convert;
      assert(convert >= 0 && convert <= 255);
      targetCount = static_cast<uint8_t>(convert);
    }
    else if (str_starts(flag, g_flag_morphTargetLayout_Position_prefix)) {
      std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Position_prefix.size()));
      ss >> convert;
      assert(convert >= 0 && convert <= 255);
      positionPosition = static_cast<int8_t>(convert);
    }
    else if (str_starts(flag, g_flag_morphTargetLayout_Normal_prefix)) {
      std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Normal_prefix.size()));
      ss >> convert;
      assert(convert >= 0 && convert <= 255);
      normalPosition = static_cast<int8_t>(convert);
    }
    else if (str_starts(flag, g_flag_morphTargetLayout_Tangent_prefix)) {
      std::stringstream ss(flag.substr(g_flag_morphTargetLayout_Tangent_prefix.size()));
      ss >> convert;
      assert(convert >= 0 && convert <= 255);
      tangentPosition = static_cast<int8_t>(convert);
    }
  }
}

int PBR_material::getMorphPositionAttributeIndexOffset() { return 1; }

int PBR_material::getMorphNormalAttributeIndexOffset() { return 1; }

int PBR_material::getMorphTangentAttributeIndexOffset() const { return requiresTangents() ? 1 : 0; }

std::vector<Vertex_attribute_element>
PBR_material::vertexInputs(const std::set<std::string>& flags) const
{
  auto vertexAttributes = Base_type::vertexInputs(flags);

  vertexAttributes.push_back(Vertex_attribute_element{{Vertex_attribute::Normal, 0},
                                                      Element_type::Vector3,
                                                      Element_component::Float});

  if (requiresTangents()) {
    vertexAttributes.push_back(
        {{Vertex_attribute::Tangent, 0}, Element_type::Vector4, Element_component::Float});
  }

  if (requiresTexCoord0()) {
    vertexAttributes.push_back(
        {{Vertex_attribute::Tex_Coord, 0}, Element_type::Vector2, Element_component::Float});
  }

  if (flags.find(g_flag_skinned) != flags.end()) {
    Element_component jointsComponent;
    if (flags.find(g_flag_skinned_joints_uint16) != flags.end())
      jointsComponent = Element_component::Unsigned_Short;
    else if (flags.find(g_flag_skinned_joints_uint32) != flags.end())
      jointsComponent = Element_component::Unsigned_Int;
    else if (flags.find(g_flag_skinned_joints_sint16) != flags.end())
      jointsComponent = Element_component::Signed_Short;
    else if (flags.find(g_flag_skinned_joints_sint32) != flags.end())
      jointsComponent = Element_component::Signed_Int;
    else
      OE_THROW(std::runtime_error("Missing joints component flag"));

    vertexAttributes.push_back(
        {{Vertex_attribute::Joints, 0}, Element_type::Vector4, jointsComponent});
    vertexAttributes.push_back(
        {{Vertex_attribute::Weights, 0}, Element_type::Vector4, Element_component::Float});
  }

  // Vertex attributes for morph targets.
  uint8_t targetCount;
  int8_t positionPosition;
  int8_t normalPosition;
  int8_t tangentPosition;
  decodeMorphTargetConfig(flags, targetCount, positionPosition, normalPosition, tangentPosition);

  if (targetCount > 8)
    OE_THROW(std::domain_error("Does not support more than 8 morph targets"));

  uint8_t morphPositionSemanticOffset = getMorphPositionAttributeIndexOffset();
  uint8_t morphNormalSemanticOffset = getMorphNormalAttributeIndexOffset();
  uint8_t morphTangentSemanticOffset = getMorphTangentAttributeIndexOffset();
  for (auto i = 0; i < targetCount; ++i) {
    if (positionPosition >= 0) {
      vertexAttributes.push_back({{Vertex_attribute::Position, morphPositionSemanticOffset++},
                                  Element_type::Vector3,
                                  Element_component::Float});
    }
    if (normalPosition >= 0) {
      vertexAttributes.push_back({{Vertex_attribute::Normal, morphNormalSemanticOffset++},
                                  Element_type::Vector3,
                                  Element_component::Float});
    }
    if (requiresTangents() && tangentPosition >= 0) {
      vertexAttributes.push_back({{Vertex_attribute::Tangent, morphTangentSemanticOffset++},
                                  Element_type::Vector4,
                                  Element_component::Float});
    }
  }

  return vertexAttributes;
}

Material::Shader_resources
PBR_material::shaderResources(const std::set<std::string>& flags,
                              const Render_light_data& renderLightData) const
{
  auto sr = Base_type::shaderResources(flags, renderLightData);

  for (size_t i = 0; i < _textures.size(); ++i) {
    const auto& texture = _textures[i];
    if (!texture)
      continue;

    sr.textures.push_back(texture);
    sr.samplerDescriptors.push_back(texture->getSamplerDescriptor());
  }

  return sr;
}

Material::Shader_compile_settings
PBR_material::vertexShaderSettings(const std::set<std::string>& flags) const
{
  auto settings = Base_type::vertexShaderSettings(flags);
  applyVertexLayoutShaderCompileSettings(settings);

  // Skinning
  if (flags.find(g_flag_skinned) != flags.end()) {
    settings.defines["VB_SKINNED"] = "1";
  }

  uint8_t targetCount;
  int8_t positionPosition;
  int8_t normalPosition;
  int8_t tangentPosition;
  decodeMorphTargetConfig(flags, targetCount, positionPosition, normalPosition, tangentPosition);

  // Add morph inputs to the vertex input struct
  if (targetCount > 0) {
    uint8_t morphPositionSemanticOffset = getMorphPositionAttributeIndexOffset();
    uint8_t morphNormalSemanticOffset = getMorphNormalAttributeIndexOffset();
    uint8_t morphTangentSemanticOffset = getMorphTangentAttributeIndexOffset();

    std::array<std::stringstream, 3> vbMorphWeightsCalcParts;
    vbMorphWeightsCalcParts[0] << "positionL = float3(positionL";
    vbMorphWeightsCalcParts[1] << "normalL = float3(Input.vNormal";
    vbMorphWeightsCalcParts[2] << "tangentL = float4(Input.vTangent.xyz";

    // Build inputs.
    std::array<std::stringstream, 3> vbMorphInputsArr;

    for (auto morphTargetIdx = 0; morphTargetIdx < targetCount; ++morphTargetIdx) {
      const auto morphTargetIdxStr = std::to_string(static_cast<int16_t>(morphTargetIdx));
      if (positionPosition != -1) {
        const auto semanticIdx = static_cast<uint8_t>(morphPositionSemanticOffset++);
        const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
        vbMorphInputsArr.at(positionPosition) << "float3 vMorphPosition" << morphTargetIdxStr
                                              << " : POSITION" << semanticIdxStr << ";";
        vbMorphWeightsCalcParts[0] << " + Input.vMorphPosition" << morphTargetIdxStr
                                   << " * g_morphWeights" << (morphTargetIdx / 4) << "["
                                   << (morphTargetIdx % 4) << "]";
        settings.morphAttributes.push_back({Vertex_attribute::Position, semanticIdx});
      }
      if (normalPosition != -1) {
        const auto semanticIdx = static_cast<uint8_t>(morphNormalSemanticOffset++);
        const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
        vbMorphInputsArr.at(normalPosition)
            << "float3 vMorphNormal" << morphTargetIdxStr << " : NORMAL" << semanticIdxStr << ";";
        vbMorphWeightsCalcParts[1] << " + Input.vMorphNormal" << morphTargetIdxStr
                                   << " * g_morphWeights" << (morphTargetIdx / 4) << "["
                                   << (morphTargetIdx % 4) << "]";
        settings.morphAttributes.push_back({Vertex_attribute::Normal, semanticIdx});
      }
      if (requiresTangents() && tangentPosition != -1) {
        const auto semanticIdx = static_cast<uint8_t>(morphTangentSemanticOffset++);
        const auto semanticIdxStr = std::to_string(static_cast<int16_t>(semanticIdx));
        vbMorphInputsArr.at(tangentPosition)
            << "float3 vMorphTangent" << morphTargetIdxStr << " : TANGENT" << semanticIdxStr << ";";
        vbMorphWeightsCalcParts[2] << " + Input.vMorphTangent" << morphTargetIdxStr
                                   << " * g_morphWeights" << (morphTargetIdx / 4) << "["
                                   << (morphTargetIdx % 4) << "]";
        settings.morphAttributes.push_back({Vertex_attribute::Tangent, semanticIdx});
      }
    }

    vbMorphWeightsCalcParts[0] << ");";
    vbMorphWeightsCalcParts[1] << ");";
    vbMorphWeightsCalcParts[2] << ", Input.vTangent.w); ";

    std::stringstream vbMorphWeightsCalc;
    vbMorphWeightsCalc << vbMorphWeightsCalcParts[0].str();
    vbMorphWeightsCalc << vbMorphWeightsCalcParts[1].str();

    if (requiresTangents())
      vbMorphWeightsCalc << vbMorphWeightsCalcParts[2].str();

    settings.defines["VB_MORPH"] = "1";
    settings.defines["VB_MORPH_INPUTS"] =
        vbMorphInputsArr[0].str() + vbMorphInputsArr[1].str() + vbMorphInputsArr[2].str();
    settings.defines["VB_MORPH_WEIGHTS_CALC"] = vbMorphWeightsCalc.str();
  }

  return settings;
}

Material::Shader_compile_settings
PBR_material::pixelShaderSettings(const std::set<std::string>& flags) const
{
  auto settings = Base_type::pixelShaderSettings(flags);

  if (_textures[BaseColor])
    settings.defines["MAP_BASECOLOR"] = "1";
  if (_textures[MetallicRoughness])
    settings.defines["MAP_METALLIC_ROUGHNESS"] = "1";
  if (_textures[Normal])
    settings.defines["MAP_NORMAL"] = "1";
  if (_textures[Emissive])
    settings.defines["MAP_EMISSIVE"] = "1";
  if (_textures[Occlusion])
    settings.defines["MAP_OCCLUSION"] = "1";

  if (flags.find(g_flag_enableDeferred) != flags.end())
    settings.defines["PS_PIPELINE_DEFERRED"] = "1";
  else
    settings.defines["PS_PIPELINE_STANDARD"] = "1";

  if (getAlphaMode() == Material_alpha_mode::Mask)
    settings.defines["ALPHA_MASK_VALUE"] = std::to_string(alphaCutoff());

  applyVertexLayoutShaderCompileSettings(settings);

  return settings;
}

void PBR_material::applyVertexLayoutShaderCompileSettings(Shader_compile_settings& settings) const
{
  settings.defines["VB_NORMAL"] = "1";
  if (requiresTexCoord0()) {
    settings.defines["VB_TEXCOORD0"] = "1";
  }
  if (requiresTangents()) {
    settings.defines["VB_TANGENT"] = "1";
  }
}

void PBR_material::updateVsConstantBufferValues(
    PBR_material_vs_constant_buffer& constants, const SSE::Matrix4& worldMatrix,
    const SSE::Matrix4& viewMatrix, const SSE::Matrix4& projMatrix,
    const Renderer_animation_data& rendererAnimationData) const
{
  constants.viewProjection = projMatrix * viewMatrix;
  constants.world = worldMatrix;
  constants.worldInvTranspose = SSE::inverse(SSE::transpose(constants.world));
  static_assert(sizeof(Float4) / sizeof(float) * 2 == Renderer_animation_data::morphWeightsSize);
  constants.morphWeights[0] = {rendererAnimationData.morphWeights[0],
                               rendererAnimationData.morphWeights[1],
                               rendererAnimationData.morphWeights[2],
                               rendererAnimationData.morphWeights[3]};
  constants.morphWeights[1] = {rendererAnimationData.morphWeights[4],
                               rendererAnimationData.morphWeights[5],
                               rendererAnimationData.morphWeights[6],
                               rendererAnimationData.morphWeights[7]};
}

void PBR_material::updatePsConstantBufferValues(PBR_material_ps_constant_buffer& constants,
                                                const SSE::Matrix4& worldMatrix,
                                                const SSE::Matrix4& /* viewMatrix */,
                                                const SSE::Matrix4& /* projMatrix */) const
{
  constants.world = worldMatrix;
  constants.baseColor = static_cast<Float4>(_baseColor);
  constants.metallicRoughness = {_metallic, _roughness, 0.0, 0.0};
  constants.emissive = {_emissive.getX(), _emissive.getY(), _emissive.getZ(), 0.0};
  // todo: remove this, it's redundant.
  constants.eyePosition = Float4{worldMatrix.getTranslation(), 0.0};
}
