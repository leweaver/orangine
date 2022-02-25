#include "OeCore/Animation_controller_component.h"
#include "OeCore/Collision.h"
#include "OeCore/Entity_graph_loader_gltf.h"
#include "OeCore/IEntity_repository.h"
#include "OeCore/FileUtils.h"
#include "OeCore/Material.h"
#include "OeCore/Mesh_data.h"
#include "OeCore/Mesh_utils.h"
#include "OeCore/Morph_weights_component.h"
#include "OeCore/PBR_material.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Skinned_mesh_component.h"
#include "OeCore/Texture.h"
#include "OeCore/Unlit_material.h"
#include "OeCore/ITexture_manager.h"
#include "OeCore/IMaterial_manager.h"
#include "OeCore/IScene_graph_manager.h"

#include <functional>
#include <wincodec.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>

using namespace std;
using namespace tinygltf;
using namespace oe;

std::string s_primAttrName_position = "POSITION";
std::string s_primAttrName_normal = "NORMAL";
std::string s_primAttrName_tangent = "TANGENT";
std::string s_primAttrName_color = "COLOR_";
std::string s_primAttrName_texCoord = "TEXCOORD_";
std::string s_primAttrName_joints = "JOINTS_";
std::string s_primAttrName_weights = "WEIGHTS_";

const map<string, Vertex_attribute_semantic> g_gltfAttributeToVertexAttributeMap = {
    {s_primAttrName_position, {Vertex_attribute::Position, 0}},
    {s_primAttrName_normal, {Vertex_attribute::Normal, 0}},
    {s_primAttrName_tangent, {Vertex_attribute::Tangent, 0}},
    {s_primAttrName_color + "0", {Vertex_attribute::Color, 0}},
    {s_primAttrName_texCoord + "0", {Vertex_attribute::Tex_coord, 0}},
    {s_primAttrName_joints + "0", {Vertex_attribute::Joints, 0}},
    {s_primAttrName_weights + "0", {Vertex_attribute::Weights, 0}},
};

const map<string, Vertex_attribute_semantic> g_gltfMorphAttributeMapping = {
    {s_primAttrName_position, {Vertex_attribute::Position, 0}},
    {s_primAttrName_normal, {Vertex_attribute::Normal, 0}},
    {s_primAttrName_tangent, {Vertex_attribute::Tangent, 0}},

};

const map<string, Animation_interpolation> g_animationSamplerInterpolationToTypeMap = {
    {"LINEAR", Animation_interpolation::Linear},
    {"STEP", Animation_interpolation::Step},
    {"CUBICSPLINE", Animation_interpolation::Cubic_spline},
};

const map<string, Animation_type> g_animationChannelTargetPathToTypeMap = {
    {"translation", Animation_type::Translation},
    {"rotation", Animation_type::Rotation},
    {"scale", Animation_type::Scale},
    {"weights", Animation_type::Morph},
};

const std::map<int, std::set<int>> g_anim_translation_allowedAccessorTypes = {
    {TINYGLTF_TYPE_VEC3, {TINYGLTF_COMPONENT_TYPE_FLOAT}}};
const std::map<int, std::set<int>>& g_anim_scale_allowedAccessorTypes =
    g_anim_translation_allowedAccessorTypes;
const std::map<int, std::set<int>> g_anim_rotation_allowedAccessorTypes = {
    {TINYGLTF_TYPE_VEC4,
     {
         TINYGLTF_COMPONENT_TYPE_FLOAT,
         TINYGLTF_COMPONENT_TYPE_BYTE,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
         TINYGLTF_COMPONENT_TYPE_SHORT,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
     }}};
const std::map<int, std::set<int>> g_anim_morph_allowedAccessorTypes = {
    {TINYGLTF_TYPE_SCALAR,
     {
         TINYGLTF_COMPONENT_TYPE_FLOAT,
         TINYGLTF_COMPONENT_TYPE_BYTE,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
         TINYGLTF_COMPONENT_TYPE_SHORT,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
     }}};
const std::array<const std::map<int, std::set<int>>*, 4> g_animTypeToAllowedAccessorTypes = {
    &g_anim_translation_allowedAccessorTypes,
    &g_anim_rotation_allowedAccessorTypes,
    &g_anim_scale_allowedAccessorTypes,
    &g_anim_morph_allowedAccessorTypes};
static_assert(
    g_animTypeToAllowedAccessorTypes.size() ==
    static_cast<unsigned>(Animation_type::Num_animation_type));
const std::map<int, std::set<int>> g_index_allowedAccessorTypes = {
    {TINYGLTF_TYPE_SCALAR,
     {
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
         TINYGLTF_COMPONENT_TYPE_BYTE,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
         TINYGLTF_COMPONENT_TYPE_SHORT,
         TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
         TINYGLTF_COMPONENT_TYPE_INT,
     }},
};
const std::map<int, std::set<int>> g_vertex_allowedAccessorTypes = {
    {TINYGLTF_TYPE_SCALAR, {/* all */}},
    {TINYGLTF_TYPE_VEC2, {/* all */}},
    {TINYGLTF_TYPE_VEC3, {/* all */}},
    {TINYGLTF_TYPE_VEC4, {/* all */}},
};
const std::map<int, std::set<int>> g_morphTarget_allowedAccessorTypes = {
    {TINYGLTF_TYPE_VEC3, {TINYGLTF_COMPONENT_TYPE_FLOAT}},
};

const std::map<int, Element_type> g_gltfType_elementType = {
    {TINYGLTF_TYPE_VEC2, Element_type::Vector2},
    {TINYGLTF_TYPE_VEC3, Element_type::Vector3},
    {TINYGLTF_TYPE_VEC4, Element_type::Vector4},
    {TINYGLTF_TYPE_MAT2, Element_type::Matrix2},
    {TINYGLTF_TYPE_MAT3, Element_type::Matrix3},
    {TINYGLTF_TYPE_MAT4, Element_type::Matrix4},
    {TINYGLTF_TYPE_SCALAR, Element_type::Scalar},
    //{ TINYGLTF_TYPE_VECTOR, Element_type:: },
    //{ TINYGLTF_TYPE_MATRIX, Element_type:: }
};
const std::map<int, Element_component> g_gltfComponent_elementComponent = {
    //{ TINYGLTF_COMPONENT_TYPE_BYTE, },
    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, Element_component::Unsigned_byte},
    //{ TINYGLTF_COMPONENT_TYPE_SHORT, },
    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, Element_component::Unsigned_short},
    //{ TINYGLTF_COMPONENT_TYPE_INT, },
    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, Element_component::Unsigned_int},
    {TINYGLTF_COMPONENT_TYPE_FLOAT, Element_component::Float},
    //{ TINYGLTF_COMPONENT_TYPE_DOUBLE, },
};

const auto g_createSimpleMeshAccessor = [](shared_ptr<Mesh_buffer> meshBuffer,
                                           Element_type type,
                                           Element_component component,
                                           size_t count,
                                           size_t stride,
                                           size_t offset) {
  return make_unique<Mesh_buffer_accessor>(
      meshBuffer,
      static_cast<uint32_t>(count),
      static_cast<uint32_t>(stride),
      static_cast<uint32_t>(offset));
};

std::function<std::unique_ptr<Mesh_vertex_buffer_accessor>(
    shared_ptr<Mesh_buffer> meshBuffer,
    Element_type type,
    Element_component component,
    size_t count,
    size_t stride,
    size_t offset)>
vertexAccessorFactory(Vertex_attribute_semantic vertexAttribute) {
  return [vertexAttribute](
             shared_ptr<Mesh_buffer> meshBuffer,
             Element_type type,
             Element_component component,
             size_t count,
             size_t stride,
             size_t offset) {
    return make_unique<Mesh_vertex_buffer_accessor>(
        meshBuffer,
        Vertex_attribute_element{vertexAttribute, type, component},
        static_cast<uint32_t>(count),
        static_cast<uint32_t>(stride),
        static_cast<uint32_t>(offset));
  };
}

struct Loader_data {
  Loader_data(
          Model& model, string&& baseDir, IWICImagingFactory* imagingFactory,
          IScene_graph_manager& sceneGraphManager, IEntity_repository& entityRepository,
          IMaterial_manager& materialManager, ITexture_manager& textureManager, IComponent_factory& componentFactory,
          bool calculateBounds)
      : model(model)
      , baseDir(std::move(baseDir))
      , imagingFactory(imagingFactory)
      , sceneGraphManager(sceneGraphManager)
      , calculateBounds(calculateBounds)
      , entityRepository(entityRepository)
      , materialManager(materialManager)
      , textureManager(textureManager)
      , componentFactory(componentFactory)
  {}

  Model& model;
  string baseDir;
  IWICImagingFactory* imagingFactory;
  IScene_graph_manager& sceneGraphManager;
  IEntity_repository& entityRepository;
  IMaterial_manager& materialManager;
  ITexture_manager& textureManager;
  IComponent_factory& componentFactory;
  map<size_t, shared_ptr<Mesh_buffer>> accessorIdxToMeshBuffers;
  vector<shared_ptr<Entity>> nodeIdxToEntity;
  bool calculateBounds;
  shared_ptr<Entity> rootEntity;
};

shared_ptr<Entity> create_entity(vector<Node>::size_type nodeIdx, Loader_data& loaderData);
void create_animation(int animIdx, Loader_data loaderData);

const char* g_pbrPropertyName_baseColorFactor = "baseColorFactor";
const char* g_pbrPropertyName_baseColorTexture = "baseColorTexture";
const char* g_pbrPropertyName_metallicFactor = "metallicFactor";
const char* g_pbrPropertyName_roughnessFactor = "roughnessFactor";
const char* g_pbrPropertyName_emissiveFactor = "emissiveFactor";
const char* g_pbrPropertyName_metallicRoughnessTexture = "metallicRoughnessTexture";
const char* g_pbrPropertyName_alphaMode = "alphaMode";
const char* g_pbrPropertyName_alphaCutoff = "alphaCutoff";

const char* g_pbrPropertyValue_alphaMode_opaque = "OPAQUE";
const char* g_pbrPropertyValue_alphaMode_mask = "MASK";
const char* g_pbrPropertyValue_alphaMode_blend = "BLEND";

Entity_graph_loader_gltf::Entity_graph_loader_gltf(IMaterial_manager& materialManager, ITexture_manager& textureManager)
    : _materialManager(materialManager)
    , _textureManager(textureManager)
{
  // Create the COM imaging factory
  ThrowIfFailed(CoCreateInstance(
      CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_imagingFactory)));
}

void Entity_graph_loader_gltf::getSupportedFileExtensions(vector<string>& extensions) const {
  extensions.emplace_back("gltf");
}

template <class TMesh_buffer_accessor>
unique_ptr<TMesh_buffer_accessor> useOrCreateBufferForAccessor(
    size_t accessorIndex,
    Loader_data& loaderData,
    const std::map<int, std::set<int>> allowedAccessorTypes,
    int expectedBufferViewTarget,
    std::function<unique_ptr<TMesh_buffer_accessor>(
        shared_ptr<Mesh_buffer>,
        Element_type type,
        Element_component component,
        size_t count,
        size_t stride,
        size_t offset)> accessorFactory) {
  if (accessorIndex >= loaderData.model.accessors.size())
    OE_THROW(domain_error("accessor[" + to_string(accessorIndex) + "] out of range."));

  const auto& accessor = loaderData.model.accessors[accessorIndex];

  // is this accessor using one of the types that are allowed?
  const auto allowedAccessorTypePos = allowedAccessorTypes.find(accessor.type);
  if (allowedAccessorTypePos == allowedAccessorTypes.end())
    OE_THROW(domain_error("Unexpected accessor type: " + to_string(accessor.type)));

  if (!allowedAccessorTypePos->second.empty() &&
      allowedAccessorTypePos->second.find(accessor.componentType) ==
          allowedAccessorTypePos->second.end()) {
    OE_THROW(
        domain_error("Unexpected accessor component type: " + to_string(accessor.componentType)));
  }

  const auto componentSizeInBytes = GetComponentSizeInBytes(accessor.componentType);
  const auto numComponentsInType = GetNumComponentsInType(accessor.type);
  const auto sourceElementSize = componentSizeInBytes * numComponentsInType;
  if (sourceElementSize <= 0) {
    OE_THROW(domain_error(
        "missing or unsupported accessor field: "s +
        (numComponentsInType == -1 ? "type" : "componentType")));
  }

  // Get the buffer view
  if (accessor.bufferView >= static_cast<int>(loaderData.model.bufferViews.size()))
    OE_THROW(domain_error("bufferView[" + to_string(accessor.bufferView) + "] out of range."));
  const auto& bufferView = loaderData.model.bufferViews.at(accessor.bufferView);

  if (expectedBufferViewTarget != 0 && bufferView.target != 0 &&
      bufferView.target != expectedBufferViewTarget) {
    OE_THROW(domain_error(
        string("Index bufferView must have type ") +
        (expectedBufferViewTarget == TINYGLTF_TARGET_ARRAY_BUFFER ? "TARGET_ARRAY_BUFFER("
                                                                  : "ARRAY_BUFFER(") +
        to_string(expectedBufferViewTarget) + ")"));
  }

  // Determine the stride - if none is provided, default to sourceElementSize.
  auto sourceStride = accessor.ByteStride(bufferView);
  if (sourceStride == -1)
    OE_THROW(std::domain_error("Invalid glTF parameters for byte stride"));
  if (sourceStride == 0)
    sourceStride = sourceElementSize;

  // does the data range defined by the accessor fit into the bufferView?
  if (accessor.byteOffset + accessor.count * sourceStride > bufferView.byteLength)
    OE_THROW(domain_error(
        "Accessor " + to_string(accessorIndex) + " exceeds maximum size of bufferView " +
        to_string(accessor.bufferView)));

  // does the data range defined by the accessor and buffer view fit into the buffer?
  const auto bufferOffset = bufferView.byteOffset + accessor.byteOffset;

  if (bufferView.buffer >= static_cast<int>(loaderData.model.buffers.size()))
    OE_THROW(domain_error("buffer[" + to_string(bufferView.buffer) + "] out of range."));
  const auto& buffer = loaderData.model.buffers.at(bufferView.buffer);

  if (bufferOffset + accessor.count * sourceStride > buffer.data.size())
    OE_THROW(domain_error(
        "BufferView " + to_string(accessor.bufferView) + " exceeds maximum size of buffer " +
        to_string(bufferView.buffer)));

  // Does the mesh buffer exist in the cache?
  const auto pos = loaderData.accessorIdxToMeshBuffers.find(accessorIndex);
  shared_ptr<Mesh_buffer> meshBuffer;
  const auto elementType = g_gltfType_elementType.at(accessor.type);
  auto elementComponent = g_gltfComponent_elementComponent.at(accessor.componentType);
  if (pos == loaderData.accessorIdxToMeshBuffers.end()) {

    // Convert index buffers from 8-bit to 32 bit.
    if (elementType == Element_type::Scalar &&
        expectedBufferViewTarget == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER &&
        (elementComponent == Element_component::Unsigned_byte ||
         elementComponent == Element_component::Signed_byte)) {
      // Transform the data to a known format
      const auto convertedIndexAccessor = mesh_utils::create_index_buffer(
          buffer.data,
          static_cast<uint32_t>(accessor.count),
          elementComponent,
          static_cast<uint32_t>(sourceStride),
          static_cast<uint32_t>(bufferOffset));
      meshBuffer = convertedIndexAccessor->buffer;
      elementComponent = convertedIndexAccessor->component;
    } else {
      // Copy the data.
      meshBuffer = mesh_utils::create_buffer(
          static_cast<uint32_t>(sourceElementSize),
          static_cast<uint32_t>(accessor.count),
          buffer.data,
          static_cast<uint32_t>(sourceStride),
          static_cast<uint32_t>(bufferOffset));
    }
    loaderData.accessorIdxToMeshBuffers[accessorIndex] = meshBuffer;
  } else {
    meshBuffer = pos->second;
  }

  // Note that the buffer we created above contains ONLY this element, thus has offset of zero, and
  // stride of elementSize.
  return accessorFactory(
      meshBuffer, elementType, elementComponent, accessor.count, sourceElementSize, 0);

  /*
       // Output stream data to the log
      if (accessor.type == TINYGLTF_TYPE_VEC2)
      {
          float *floatData = reinterpret_cast<float*>(meshBuffer->m_data);
          for (size_t i = 0; i < accessor.count; i++)
          {
              LOG(G3LOG_DEBUG) << to_string(i) << ": " << floatData[0] << ", " << floatData[1];
              floatData += 2;
          }
      }
      */

  /*
  if (vertexAttribute == VertexAttribute::VA_POSITION && elementSize == 3 * sizeof(float))
  {
      float *vec3data = reinterpret_cast<float *>(meshBuffer->m_data);
      for (UINT i = 0; i < accessor.count; ++i) {
          // Invert the Z axis
          vec3data[2] *= -1;
          vec3data += 3;
      }
  }
  */
}

SSE::Matrix4 createMatrix4FromGltfArray(const vector<double>& m) {
  // glTF matrices are Column Major
  // SSE matrices are Column Major

  return SSE::Matrix4(
      {static_cast<float>(m[0]),
       static_cast<float>(m[1]),
       static_cast<float>(m[2]),
       static_cast<float>(m[3])},
      {static_cast<float>(m[4]),
       static_cast<float>(m[5]),
       static_cast<float>(m[6]),
       static_cast<float>(m[7])},
      {static_cast<float>(m[8]),
       static_cast<float>(m[9]),
       static_cast<float>(m[10]),
       static_cast<float>(m[11])},
      {static_cast<float>(m[12]),
       static_cast<float>(m[13]),
       static_cast<float>(m[14]),
       static_cast<float>(m[15])});
}

std::vector<SSE::Matrix4> createMatrix4ArrayFromAccessor(
    Loader_data& loaderData,
    int accessorIndex) {
  // Inverse bind matrices
  const auto matricesAccessor = useOrCreateBufferForAccessor<Mesh_buffer_accessor>(
      accessorIndex,
      loaderData,
      {{TINYGLTF_TYPE_MAT4, {TINYGLTF_COMPONENT_TYPE_FLOAT}}},
      0,
      g_createSimpleMeshAccessor);

  std::vector<SSE::Matrix4> matrices;
  matrices.reserve(matricesAccessor->count);

  for (auto i = 0u; i < matricesAccessor->count; ++i) {
    const auto m = reinterpret_cast<const float*>(matricesAccessor->getIndexed(i));
    matrices.push_back(SSE::Matrix4(
        {m[0], m[1], m[2], m[3]},
        {m[4], m[5], m[6], m[7]},
        {m[8], m[9], m[10], m[11]},
        {m[12], m[13], m[14], m[15]}));
  }

  return matrices;
}

vector<shared_ptr<Entity>> Entity_graph_loader_gltf::loadFile(
    string_view filePath,
    IScene_graph_manager& sceneGraphManager,
    IEntity_repository& entityRepository,
    IComponent_factory& componentFactory,
    bool calculateBounds) const {
  vector<shared_ptr<Entity>> entities;
  Model model;
  TinyGLTF loader;
  string err;
  string warn;

  const auto filePathStr = string(filePath);
  LOG(INFO) << "Loading entity graph (glTF): " << filePathStr;

  auto gltfAscii = get_file_contents(filePathStr.c_str());

  std::string baseDir;
  {
    const auto lastSlashPos = filePathStr.find_last_of("/\\");
    if (lastSlashPos != std::string::npos) {
      baseDir = filePathStr.substr(0, lastSlashPos);
    }
  }

  const auto filename = filePathStr.substr(baseDir.size());
  const auto ret = loader.LoadASCIIFromString(
      &model,
      &err,
      &warn,
      gltfAscii.c_str(),
      static_cast<unsigned>(gltfAscii.length()),
      baseDir);
  if (!err.empty()) {
    OE_THROW(domain_error(err));
  }
  if (!warn.empty()) {
    LOG(WARNING) << filename << ": " << warn;
  }

  if (!ret) {
    OE_THROW(domain_error("Failed to parse glTF: unknown error."));
  }

  if (model.defaultScene >= static_cast<int>(model.scenes.size()) || model.defaultScene < 0)
    OE_THROW(domain_error("Failed to parse glTF: defaultScene points to an invalid scene index"));

  const auto& scene = model.scenes[model.defaultScene];
  if (baseDir.empty())
    baseDir = ".";
  Loader_data loaderData(
          model, move(baseDir), _imagingFactory.Get(), sceneGraphManager, entityRepository, _materialManager,
          _textureManager, componentFactory, calculateBounds);

  // Load Entities
  loaderData.rootEntity = entityRepository.instantiate(filename, loaderData.sceneGraphManager, componentFactory);
  for (auto nodeIdx : scene.nodes) {
    auto entity = create_entity(nodeIdx, loaderData);
    entity->setParent(*loaderData.rootEntity);
    entities.push_back(entity);
  }

  // Load Skins
  const auto numSkins = static_cast<int>(model.skins.size());
  for (size_t idx = 0; idx < loaderData.nodeIdxToEntity.size(); ++idx) {
    const auto entity = loaderData.nodeIdxToEntity[idx];
    const auto skinIdx = model.nodes[idx].skin;
    if (skinIdx <= -1)
      continue;

    if (skinIdx >= numSkins) {
      OE_THROW(std::domain_error(
          "Node[" + to_string(idx) + "] points to invalid skin index: " + to_string(skinIdx)));
    }

    try {
      const auto& skin = model.skins[skinIdx];

      const auto numNodes = static_cast<int>(loaderData.nodeIdxToEntity.size());
      for (const auto& childEntity : entity->children()) {
        if (!childEntity->getFirstComponentOfType<Mesh_data_component>())
          continue;

        auto& skinnedMeshComponent = childEntity->addComponent<Skinned_mesh_component>();

        {
          std::vector<SSE::Matrix4> inverseBindMatrices =
              createMatrix4ArrayFromAccessor(loaderData, skin.inverseBindMatrices);

          // Nodes that form the skeleton hierarchy.
          std::vector<std::shared_ptr<Entity>> joints;
          int num_joints = static_cast<int>(model.nodes.size());
          for (const int jointNodeIdx : skin.joints) {
            if (jointNodeIdx < 0 || jointNodeIdx >= num_joints)
              OE_THROW(std::domain_error("invalid joint index: " + to_string(jointNodeIdx)));

            auto jointEntity = loaderData.nodeIdxToEntity.at(jointNodeIdx);
            joints.push_back(jointEntity);
          }
          skinnedMeshComponent.setJoints(move(joints));
          skinnedMeshComponent.setInverseBindMatrices(move(inverseBindMatrices));
        }

        // Node that the skeleton is rooted from
        if (skin.skeleton > -1) {
          if (skin.skeleton >= numNodes)
            OE_THROW(std::domain_error("invalid skeleton index: " + to_string(skin.skeleton)));

          skinnedMeshComponent.setSkeletonTransformRoot(
              loaderData.nodeIdxToEntity.at(skin.skeleton));
        }
      }
    } catch (std::exception& ex) {
      OE_THROW(std::domain_error("Skin[" + to_string(skinIdx) + "]: "s + ex.what()));
    }
  }

  // Load Animations
  if (!model.animations.empty()) {
    loaderData.rootEntity->addComponent<Animation_controller_component>();
    for (auto animIdx = 0u; animIdx < model.animations.size(); ++animIdx) {
      try {
        create_animation(animIdx, loaderData);
      } catch (std::exception& ex) {
        OE_THROW(std::domain_error("Animation[" + to_string(animIdx) + "]: "s + ex.what()));
      }
    }
  }

  return entities;
}

bool tryParseAddressMode(int gltfWrap, Sampler_texture_address_mode& parsedValue) {
  switch (gltfWrap) {
  case TINYGLTF_TEXTURE_WRAP_REPEAT:
    parsedValue = Sampler_texture_address_mode::Wrap;
    return true;
  case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
    parsedValue = Sampler_texture_address_mode::Mirror;
    return true;
  case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
    parsedValue = Sampler_texture_address_mode::Clamp;
    return true;
  default:
    return false;
  }
}

bool tryParseFilterType(int gltfFilter, Sampler_filter_type& parsedValue) {
  switch (gltfFilter) {
  case TINYGLTF_TEXTURE_FILTER_LINEAR:
    parsedValue = Sampler_filter_type::Linear;
    return true;
  case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    parsedValue = Sampler_filter_type::Linear_mipmap_linear;
    return true;
  case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    parsedValue = Sampler_filter_type::Linear_mipmap_point;
    return true;
  case TINYGLTF_TEXTURE_FILTER_NEAREST:
    parsedValue = Sampler_filter_type::Point;
    return true;
  case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    parsedValue = Sampler_filter_type::Point_mipmap_linear;
    return true;
  case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    parsedValue = Sampler_filter_type::Point_mipmap_point;
    return true;
  default:
    return false;
  }
}

shared_ptr<oe::Texture> try_create_texture(
    const Loader_data& loaderData,
    const tinygltf::Material& gltfMaterial,
    const std::string& textureName) {
  int gltfTextureIndex;

  // Look for PBR textures in 'values'
  auto paramPos = gltfMaterial.values.find(textureName);
  if (paramPos != gltfMaterial.values.end()) {
    const auto& param = paramPos->second;
    const auto indexPos = param.json_double_value.find("index");
    if (indexPos == param.json_double_value.end())
      OE_THROW(domain_error("missing property 'index' from " + textureName));

    gltfTextureIndex = static_cast<int>(indexPos->second);
  } else {
    paramPos = gltfMaterial.additionalValues.find(textureName);
    if (paramPos == gltfMaterial.additionalValues.end())
      return nullptr;

    const auto& param = paramPos->second;
    const auto indexPos = param.json_double_value.find("index");
    if (indexPos == param.json_double_value.end())
      OE_THROW(domain_error("missing property 'index' from " + textureName));

    gltfTextureIndex = static_cast<int>(indexPos->second);
  }

  assert(loaderData.model.textures.size() < INT_MAX);
  if (gltfTextureIndex < 0 ||
      gltfTextureIndex >= static_cast<int>(loaderData.model.textures.size()))
    OE_THROW(domain_error(
        "invalid texture index '" + to_string(gltfTextureIndex) + "' for " + textureName));

  const auto& gltfTexture = loaderData.model.textures.at(gltfTextureIndex);

  auto samplerDescriptor = Sampler_descriptor(Default_values());
  if (gltfTexture.sampler >= 0) {
    const auto& gltfSampler = loaderData.model.samplers.at(gltfTexture.sampler);
    {
      const auto validWrap = tryParseAddressMode(gltfSampler.wrapS, samplerDescriptor.wrapU) &&
                             tryParseAddressMode(gltfSampler.wrapT, samplerDescriptor.wrapV);
      if (!validWrap) {
        OE_THROW(
            domain_error("image " + to_string(gltfTexture.source) + " invalid sample wrap value"));
      }
    }

    {
      const auto validFilterType =
          tryParseFilterType(gltfSampler.minFilter, samplerDescriptor.minFilter) &&
          tryParseFilterType(gltfSampler.magFilter, samplerDescriptor.magFilter);

      if (!validFilterType) {
        OE_THROW(domain_error(
            "image " + to_string(gltfTexture.source) + " invalid sample filter value"));
      }
    }
  }

  const auto& gltfImage = loaderData.model.images.at(gltfTexture.source);

  if (!gltfImage.uri.empty()) {
    const auto filename = loaderData.baseDir + "\\" + gltfImage.uri;
    return loaderData.textureManager.createTextureFromFile(filename, samplerDescriptor);
  }
  if (!gltfImage.mimeType.empty()) {
    OE_THROW(runtime_error("not implemented"));
  }
  OE_THROW(domain_error(
      "image " + to_string(gltfTexture.source) + "must have either a uri or mimeType"));
}

shared_ptr<oe::Material> create_material(
    const Primitive& prim,
    Loader_data& loaderData) {
  auto material = std::make_shared<PBR_material>();
  if (prim.material < 0) {
    return material;
  }

  if (prim.material >= loaderData.model.materials.size()) {
    OE_THROW(domain_error(
        "refers to material index " + to_string(prim.material) + " which doesn't exist."));
  }

  const auto& gltfMaterial = loaderData.model.materials[prim.material];
  const auto withParam = [gltfMaterial](
                             const string& name,
                             std::function<void(const string&, const Parameter&)> found,
                             std::function<void(const string&)> notFound) {
    auto paramPos = gltfMaterial.values.find(name);
    if (paramPos != gltfMaterial.values.end()) {
      found(name, paramPos->second);
      return true;
    }

    paramPos = gltfMaterial.additionalValues.find(name);
    if (paramPos != gltfMaterial.additionalValues.end()) {
      found(name, paramPos->second);
      return true;
    }

    notFound(name);
    return false;
  };

  const auto withScalarParam =
      [&withParam,
       &material](const string& name, float defaultValue, function<void(float)> setter) {
        withParam(
            name,
            [&setter](auto name, auto param) {
              if (!param.has_number_value)
                OE_THROW(domain_error(
                    "Failed to parse glTF: expected material " + name + " to be scalar"));
              setter(static_cast<float>(param.number_value));
            },
            [&setter, defaultValue](auto) { setter(defaultValue); });
      };

  const auto withColorParam =
      [&withParam,
       &material](const string& name, const Color& defaultValue, function<void(Color)> setter) {
        withParam(
            name,
            [&setter](auto name, auto param) {
              if (param.number_array.size() == 3) {
                setter(Color(
                    static_cast<float>(param.number_array[0]),
                    static_cast<float>(param.number_array[1]),
                    static_cast<float>(param.number_array[2]),
                    1.0f));
              } else if (param.number_array.size() == 4) {
                setter(Color(
                    static_cast<float>(param.number_array[0]),
                    static_cast<float>(param.number_array[1]),
                    static_cast<float>(param.number_array[2]),
                    static_cast<float>(param.number_array[3])));
              } else
                OE_THROW(domain_error(
                    "Failed to parse glTF: expected material " + name +
                    " to be a 3 or 4 element array"));
            },
            [&setter, defaultValue](auto) { setter(defaultValue); });
      };

  // Numeric/Color Parameters
  withScalarParam(
      g_pbrPropertyName_metallicFactor,
      1.0f,
      bind(&PBR_material::setMetallicFactor, material.get(), std::placeholders::_1));
  withScalarParam(
      g_pbrPropertyName_roughnessFactor,
      1.0f,
      bind(&PBR_material::setRoughnessFactor, material.get(), std::placeholders::_1));
  withScalarParam(
      g_pbrPropertyName_alphaCutoff,
      0.5f,
      bind(&PBR_material::setAlphaCutoff, material.get(), std::placeholders::_1));
  withColorParam(
      g_pbrPropertyName_baseColorFactor,
      Colors::White,
      bind(&PBR_material::setBaseColor, material.get(), std::placeholders::_1));
  withColorParam(
      g_pbrPropertyName_emissiveFactor,
      Colors::Black,
      bind(&PBR_material::setEmissiveFactor, material.get(), std::placeholders::_1));

  // Alpha Mode
  withParam(
      g_pbrPropertyName_alphaMode,
      [&material](const string& name, const Parameter& param) {
        if (param.string_value == g_pbrPropertyValue_alphaMode_mask)
          material->setAlphaMode(Material_alpha_mode::Mask);
        else if (param.string_value == g_pbrPropertyValue_alphaMode_blend)
          material->setAlphaMode(Material_alpha_mode::Blend);
        else if (param.string_value == g_pbrPropertyValue_alphaMode_opaque)
          material->setAlphaMode(Material_alpha_mode::Opaque);
        else {
          OE_THROW(domain_error(
              "Failed to parse glTF: expected material " + name +
              " to be one of:" + g_pbrPropertyValue_alphaMode_mask + ", " +
              g_pbrPropertyValue_alphaMode_blend + ", " + g_pbrPropertyValue_alphaMode_opaque));
        }
      },
      [&material](const string&) { material->setAlphaMode(Material_alpha_mode::Opaque); });

  // PBR Textures
  material->setBaseColorTexture(try_create_texture(loaderData, gltfMaterial, "baseColorTexture"));
  material->setMetallicRoughnessTexture(
      try_create_texture(loaderData, gltfMaterial, "metallicRoughnessTexture"));

  // Material Textures
  material->setNormalTexture(try_create_texture(loaderData, gltfMaterial, "normalTexture"));
  material->setOcclusionTexture(try_create_texture(loaderData, gltfMaterial, "occlusionTexture"));
  material->setEmissiveTexture(try_create_texture(loaderData, gltfMaterial, "emissiveTexture"));

  return material;
}

void setEntityTransform(Entity& entity, const Node& node) {
  if (node.matrix.size() == 16) {
    entity.setTransform(createMatrix4FromGltfArray(node.matrix));
  } else {
    if (node.translation.size() == 3) {
      entity.setPosition(
          {static_cast<float>(node.translation[0]),
           static_cast<float>(node.translation[1]),
           static_cast<float>(node.translation[2])});
    }
    if (node.rotation.size() == 4) {
      entity.setRotation(
          {static_cast<float>(node.rotation[0]),
           static_cast<float>(node.rotation[1]),
           static_cast<float>(node.rotation[2]),
           static_cast<float>(node.rotation[3])});
    }
    if (node.scale.size() == 3) {
      entity.setScale(
          {static_cast<float>(node.scale[0]),
           static_cast<float>(node.scale[1]),
           static_cast<float>(node.scale[2])});
    }
  }
}

bool loadJointsWeights(
    int index,
    const Primitive& prim,
    Loader_data& loaderData,
    Mesh_data& meshData) {
  const auto jointsAttrName = s_primAttrName_joints + to_string(index);
  const auto weightsAttrName = s_primAttrName_weights + to_string(index);

  const auto& animJointAttrPos = prim.attributes.find(jointsAttrName);
  const auto& animWeightsAttrPos = prim.attributes.find(weightsAttrName);
  if (animJointAttrPos != prim.attributes.end() && animWeightsAttrPos != prim.attributes.end()) {
    const auto loadAccessor = [&loaderData, &meshData](
                                  Vertex_attribute_semantic vertexAttribute,
                                  const std::map<int, std::set<int>>& allowedTypes,
                                  decltype(prim.attributes.find(string())) pos,
                                  const string& attrName) {
      try {
        // Animation Joints
        auto accessor = useOrCreateBufferForAccessor<Mesh_vertex_buffer_accessor>(
            pos->second, loaderData, allowedTypes, 0, vertexAccessorFactory(vertexAttribute));
        meshData.vertexBufferAccessors[vertexAttribute] = move(accessor);
      } catch (const exception& e) {
        OE_THROW(domain_error("Error in attribute " + attrName + ": " + e.what()));
      }
    };

    loadAccessor(
        {Vertex_attribute::Joints, 0},
        {{TINYGLTF_TYPE_VEC4,
          {TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT}}},
        animJointAttrPos,
        jointsAttrName);
    loadAccessor(
        {Vertex_attribute::Weights, 0},
        {{TINYGLTF_TYPE_VEC4,
          {TINYGLTF_COMPONENT_TYPE_FLOAT,
           TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
           TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT}}},
        animWeightsAttrPos,
        weightsAttrName);

    return true;
  }
  return false;
}

shared_ptr<Entity> create_entity(
    vector<Node>::size_type nodeIdx,
    Loader_data& loaderData) {
  const auto& node = loaderData.model.nodes.at(nodeIdx);
  auto rootEntity = loaderData.entityRepository.instantiate(node.name, loaderData.sceneGraphManager, loaderData.componentFactory);
  if (loaderData.nodeIdxToEntity.size() <= nodeIdx) {
    loaderData.nodeIdxToEntity.resize(nodeIdx + 1);
  }
  loaderData.nodeIdxToEntity[nodeIdx] = rootEntity;

  LOG(G3LOG_DEBUG) << "Creating entity for glTF node '" << node.name << "'";

  // Transform
  setEntityTransform(*rootEntity, node);

  // Create MeshData
  if (node.mesh > -1) {
    const auto& mesh = loaderData.model.meshes.at(node.mesh);
    const auto primitiveCount = mesh.primitives.size();
    for (size_t primIdx = 0; primIdx < primitiveCount; ++primIdx) {
      const auto& prim = mesh.primitives.at(primIdx);
      const auto primitiveName = mesh.name + " primitive " + to_string(primIdx);
      LOG(G3LOG_DEBUG) << "Creating entity for glTF mesh " << primitiveName;
      auto primitiveEntity = loaderData.entityRepository.instantiate(primitiveName, loaderData.sceneGraphManager, loaderData.componentFactory);

      primitiveEntity->setParent(*rootEntity.get());
      auto& meshDataComponent = primitiveEntity->addComponent<Mesh_data_component>();

      // Determine the vertex layout
      vector<Vertex_attribute_element> meshLayoutAttributes;
      const auto numAccessors = static_cast<int>(loaderData.model.accessors.size());
      for (const auto& attr : prim.attributes) {
        const auto vaPos = g_gltfAttributeToVertexAttributeMap.find(attr.first);
        if (vaPos == g_gltfAttributeToVertexAttributeMap.end()) {
          LOG(WARNING) << "Skipping unsupported attribute: " << attr.first;
          continue;
        }

        if (attr.second >= numAccessors) {
          OE_THROW(std::domain_error("Invalid attribute accessor index: " + attr.first));
        }
        const auto& accessor = loaderData.model.accessors[attr.second];

        const auto accessorTypePos = g_gltfType_elementType.find(accessor.type);
        const auto accessorComponentTypePos =
            g_gltfComponent_elementComponent.find(accessor.componentType);

        if (accessorTypePos == g_gltfType_elementType.end()) {
          OE_THROW(std::domain_error(std::string("Unsupported gltf accessor type: ") + to_string(accessor.type)));
        }
        if (accessorComponentTypePos == g_gltfComponent_elementComponent.end()) {
          OE_THROW(std::domain_error(std::string("Unsupported gltf accessor component type: ") + to_string(accessor.componentType)));
        }

        meshLayoutAttributes.push_back(Vertex_attribute_element{
            vaPos->second, accessorTypePos->second, accessorComponentTypePos->second});
      }

      vector<Vertex_attribute_semantic> morphTargetLayout;
      if (!prim.targets.empty()) {
        for (const auto& morphTargetEntry : prim.targets[0]) {
          const auto attrPos = g_gltfMorphAttributeMapping.find(morphTargetEntry.first);
          if (attrPos == g_gltfMorphAttributeMapping.end()) {
            OE_THROW(std::domain_error("Unknown morph attribute: " + morphTargetEntry.first));
          }
          morphTargetLayout.push_back(attrPos->second);
        }
      }

      if (prim.targets.size() > UINT8_MAX) {
        OE_THROW(std::domain_error("Too many morph targets"));
      }
      auto meshData = std::make_shared<Mesh_data>(Mesh_vertex_layout(
          meshLayoutAttributes, morphTargetLayout, static_cast<uint8_t>(prim.targets.size())));
      meshDataComponent.setMeshData(meshData);

      try {
        const auto material = create_material(prim, loaderData);

        // Read Index
        try {
          // Index
          auto indexBufferAccessor = useOrCreateBufferForAccessor<Mesh_index_buffer_accessor>(
              prim.indices,
              loaderData,
              g_index_allowedAccessorTypes,
              TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER,
              [](shared_ptr<Mesh_buffer> meshBuffer,
                 Element_type,
                 Element_component component,
                 size_t count,
                 size_t stride,
                 size_t offset) {
                return make_unique<Mesh_index_buffer_accessor>(
                    meshBuffer,
                    component,
                    static_cast<uint32_t>(count),
                    static_cast<uint32_t>(stride),
                    static_cast<uint32_t>(offset));
              });
          meshData->indexBufferAccessor = move(indexBufferAccessor);
        } catch (const exception& e) {
          OE_THROW(std::domain_error(string("Error in index buffer: ") + e.what()));
        }

        auto generateNormals = false, generateTangents = false, generateBiTangents = false;
        for (const auto& attr : prim.attributes) {
          const auto vaPos = g_gltfAttributeToVertexAttributeMap.find(attr.first);
          if (vaPos == g_gltfAttributeToVertexAttributeMap.end()) {
            OE_THROW(std::domain_error("Unexpected attribute: " + attr.first));
          }

          const auto& vertexAttribute = vaPos->second;
          try {

            // Vertex
            auto vertexBufferAccessor = useOrCreateBufferForAccessor<Mesh_vertex_buffer_accessor>(
                attr.second,
                loaderData,
                g_vertex_allowedAccessorTypes,
                TINYGLTF_TARGET_ARRAY_BUFFER,
                vertexAccessorFactory(vertexAttribute));

            meshData->vertexBufferAccessors[vertexAttribute] = move(vertexBufferAccessor);
          } catch (const exception& e) {
            OE_THROW(std::domain_error(
                "Error in attribute " + Vertex_attribute_meta::vsInputName(vertexAttribute) + ": " +
                e.what()));
          }
        }

        // Animation data
        if (loadJointsWeights(0, prim, loaderData, *meshData)) {
          if (loadJointsWeights(1, prim, loaderData, *meshData)) {
            OE_THROW(std::exception("loader does not support more than one weights stream"));
          }
        }

        // Add this component last, to make sure there wasn't an error loading!
        auto& renderableComponent = primitiveEntity->addComponent<Renderable_component>();
        renderableComponent.setMaterial(material);

        // Calculate bounds?
        if (loaderData.calculateBounds) {
          const auto& vertexBufferAccessor =
              meshData->vertexBufferAccessors.at({Vertex_attribute::Position, 0});
          assert(vertexBufferAccessor->stride >= sizeof(Float3));
          auto boundingSphere = oe::BoundingSphere::createFromPoints(
              reinterpret_cast<Float3*>(vertexBufferAccessor->buffer->data),
              vertexBufferAccessor->count,
              vertexBufferAccessor->stride);

          primitiveEntity->setBoundSphere(boundingSphere);
        }

        // Morph Targets
        if (!prim.targets.empty()) {
          auto morphWeights = node.weights;
          if (morphWeights.empty())
            morphWeights = mesh.weights;
          if (morphWeights.empty())
            morphWeights.resize(prim.targets.size(), 0.0);

          if (morphWeights.size() != prim.targets.size())
            OE_THROW(std::domain_error("Size of weights must equal size of targets, or be unset."));

          // Check the morph targets length
          const auto targetSize = prim.targets[0].size();
          for (const auto& target : prim.targets) {
            if (targetSize != target.size())
              OE_THROW(std::domain_error("Size of each target must be the same."));

            std::vector<std::unique_ptr<Mesh_vertex_buffer_accessor>> morphBufferAccessors;
            for (const auto& targetAttributeAccessor : target) {
              const auto vertexAttribute =
                  g_gltfMorphAttributeMapping.at(targetAttributeAccessor.first);
              auto accessor = useOrCreateBufferForAccessor<Mesh_vertex_buffer_accessor>(
                  targetAttributeAccessor.second,
                  loaderData,
                  g_morphTarget_allowedAccessorTypes,
                  0,
                  vertexAccessorFactory(vertexAttribute));
              morphBufferAccessors.push_back(move(accessor));
            }

            meshData->attributeMorphBufferAccessors.push_back(move(morphBufferAccessors));
          }

          auto& morphWeightsComponent = primitiveEntity->addComponent<Morph_weights_component>();
          assert(prim.targets.size() <= Morph_weights_component::maxMorphTargetCount());

          morphWeightsComponent.setMorphTargetCount(static_cast<uint8_t>(prim.targets.size()));
          std::copy(
              morphWeights.begin(),
              morphWeights.end(),
              morphWeightsComponent.morphWeights().begin());
        }
      } catch (const exception& e) {
        OE_THROW(std::domain_error(
            string("Primitive[") + to_string(primIdx) + "] is malformed. (" + e.what() + ")"));
      }
    }
  }

  for (auto childIdx : node.children) {
    auto childEntity = create_entity(childIdx, loaderData);
    childEntity->setParent(*rootEntity.get());
  }

  return rootEntity;
}

void create_animation(int animIdx, Loader_data loaderData) {
  const auto& gltfAnimation = loaderData.model.animations[animIdx];
  auto animationController =
      loaderData.rootEntity->getFirstComponentOfType<Animation_controller_component>();

  if (gltfAnimation.channels.empty())
    return;

  std::map<int, std::shared_ptr<std::vector<float>>> samplerInputToKeyframeTimes;
  const auto populateSamplerInputKeyframeTimes =
      [&gltfAnimation, &loaderData](int accessorIdx, std::vector<float>& keyframeTimes) {
        // Animation sampler
        const auto accessor = useOrCreateBufferForAccessor<Mesh_buffer_accessor>(
            accessorIdx,
            loaderData,
            {{TINYGLTF_TYPE_SCALAR, {TINYGLTF_COMPONENT_TYPE_FLOAT}}},
            0,
            g_createSimpleMeshAccessor);
        keyframeTimes.clear();
        keyframeTimes.reserve(accessor->count);
        for (auto i = 0u; i < accessor->count; i++) {
          const auto offset = accessor->offset + accessor->stride * i;
          const auto timeArray = reinterpret_cast<float*>(accessor->buffer->data + offset);
          keyframeTimes.push_back(*timeArray);
        }
      };

  // Load the channel configurations
  auto animation = std::make_unique<Animation_controller_component::Animation>();
  auto animationName = gltfAnimation.name;
  if (animationName.empty() || animationController->containsAnimationByName(animationName)) {
    animationName = "glTF animation " + to_string(animIdx);
  }

  auto& states = animationController->activeAnimations.insert({animationName, {}}).first->second;
  const auto numNodes = static_cast<int>(loaderData.nodeIdxToEntity.size());
  for (size_t channelIdx = 0; channelIdx < gltfAnimation.channels.size(); ++channelIdx) {
    const auto& channel = gltfAnimation.channels[channelIdx];
    if (channel.target_node <= -1) {
      LOG(DEBUG) << "Missing target node on animation channel at index " + to_string(channelIdx) +
                        " - ignoring.";
      continue;
    }

    if (channel.target_node >= numNodes)
      OE_THROW(std::domain_error(
          "Invalid animation channel at index " + to_string(channelIdx) +
          " - references unknown node " + to_string(channel.target_node)));

    const auto& targetEntity = loaderData.nodeIdxToEntity[channel.target_node];
    if (!targetEntity)
      OE_THROW(std::domain_error(
          "Invalid animation channel at index " + to_string(channelIdx) + " - references node " +
          to_string(channel.target_node) + " that doesn't belong to the scene."));

    assert(animationController);

    const auto& sampler = gltfAnimation.samplers[channel.sampler];

    // Interpolation type
    const auto interpolationTypePos =
        g_animationSamplerInterpolationToTypeMap.find(sampler.interpolation);
    if (interpolationTypePos == g_animationSamplerInterpolationToTypeMap.end())
      OE_THROW(std::domain_error(
          "Unknown animation sampler interpolation type: " + sampler.interpolation));
    const auto interpolationType = interpolationTypePos->second;

    // Animation property
    const auto animationTypePos = g_animationChannelTargetPathToTypeMap.find(channel.target_path);
    if (animationTypePos == g_animationChannelTargetPathToTypeMap.end())
      OE_THROW(std::domain_error("Unknown animation channel target_path: " + channel.target_path));
    const auto animationType = animationTypePos->second;

    // Keyframe Times
    const auto samplerInputToKeyframeTimesPos = samplerInputToKeyframeTimes.find(sampler.input);
    decltype(samplerInputToKeyframeTimesPos->second) keyframeTimes;
    if (samplerInputToKeyframeTimesPos == samplerInputToKeyframeTimes.end()) {
      try {
        keyframeTimes = std::make_shared<std::vector<float>>();
        populateSamplerInputKeyframeTimes(sampler.input, *keyframeTimes);
        samplerInputToKeyframeTimes[sampler.input] = keyframeTimes;
      } catch (std::exception& ex) {
        OE_THROW(std::domain_error(
            "Failed to load sampler " + to_string(channel.sampler) + ". " + ex.what()));
      };
    } else {
      keyframeTimes = samplerInputToKeyframeTimesPos->second;
    }

    // Keyframe Values
    const auto allowedAccessorTypes =
        g_animTypeToAllowedAccessorTypes[static_cast<unsigned>(animationType)];
    std::unique_ptr<Mesh_buffer_accessor> keyframeValues;
    try {
      keyframeValues = useOrCreateBufferForAccessor<Mesh_buffer_accessor>(
          sampler.output, loaderData, *allowedAccessorTypes, 0, g_createSimpleMeshAccessor);
    } catch (std::exception& ex) {
      OE_THROW(
          std::domain_error("Failed to create animation keyframe values accessor: "s + ex.what()));
    }

    uint8_t valuesPerKeyFrame = 1;
    if (interpolationType == Animation_interpolation::Cubic_spline)
      valuesPerKeyFrame *= 3;

    // For morph animations, need to add a single channel per primitive as the
    // Morph_weights_component is on the child entities.
    std::vector<std::shared_ptr<Entity>> targetEntities;
    if (animationType == Animation_type::Morph) {
      if (targetEntity->children().empty()) {
        LOG(WARNING) << "glTF Morph animation node must have at least 1 primitive.";
        continue;
      }
      const auto morphComponent =
          targetEntity->children().at(0)->getFirstComponentOfType<Morph_weights_component>();
      assert(morphComponent->morphTargetCount() <= 8 && morphComponent->morphTargetCount() >= 1);
      valuesPerKeyFrame *= morphComponent->morphTargetCount();

      std::copy(
          targetEntity->children().begin(),
          targetEntity->children().end(),
          std::back_inserter(targetEntities));
    } else {
      targetEntities.push_back(targetEntity);
    }

    for (const auto channelTargetEntity : targetEntities) {
      auto animationChannel = std::make_unique<Animation_controller_component::Animation_channel>();
      animationChannel->animationType = animationType;
      animationChannel->interpolationType = interpolationType;
      animationChannel->valuesPerKeyFrame = valuesPerKeyFrame;
      animationChannel->targetNode = channelTargetEntity;
      animationChannel->keyframeTimes = keyframeTimes;
      animationChannel->keyframeValues = move(keyframeValues);
      animation->channels.push_back(move(animationChannel));
      states.push_back({});
    }
  }

  animationController->addAnimation(animationName, move(animation));
}
