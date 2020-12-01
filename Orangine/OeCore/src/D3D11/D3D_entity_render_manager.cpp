#include "pch.h"

#include "../Entity_render_manager.h"

#include "OeCore/DeviceResources.h"
#include "OeCore/Scene.h"

#include "D3D_device_repository.h"
#include "D3D_Renderer_data.h"

#include "CommonStates.h"
#include "DirectX_utils.h"
#include "OeCore/Mesh_utils.h"

#include <cinttypes>

using namespace DirectX;

namespace oe::internal {
class D3D_entity_render_manager : public oe::internal::Entity_render_manager {
 public:
  D3D_entity_render_manager(
      Scene& scene,
      std::shared_ptr<IMaterial_repository> materialRepository,
      std::shared_ptr<D3D_device_repository> deviceRepository)
      : Entity_render_manager(scene, materialRepository), _deviceRepository(deviceRepository) {}

  inline DX::DeviceResources& deviceResources() const {
    return _deviceRepository->deviceResources();
  }

  void loadRendererDataToDeviceContext(
      const Renderer_data& rendererData,
      const Material_context& context) override {
    const auto deviceContext = deviceResources().GetD3DDeviceContext();

    // Send the buffers to the input assembler in the order that the material context requires.
    if (!rendererData.vertexBuffers.empty()) {
      _bufferArraySet.bufferArray.clear();
      _bufferArraySet.strideArray.clear();
      _bufferArraySet.offsetArray.clear();

      for (const auto vsInput : context.compilerInputs.vsInputs) {
        const auto accessorPos = rendererData.vertexBuffers.find(vsInput.semantic);
        assert(accessorPos != rendererData.vertexBuffers.end());
        const auto& accessor = accessorPos->second;
        _bufferArraySet.bufferArray.push_back(accessor->buffer->d3dBuffer);
        _bufferArraySet.strideArray.push_back(accessor->stride);
        _bufferArraySet.offsetArray.push_back(accessor->offset);
      }

      deviceContext->IASetVertexBuffers(
          0,
          static_cast<UINT>(_bufferArraySet.bufferArray.size()),
          _bufferArraySet.bufferArray.data(),
          _bufferArraySet.strideArray.data(),
          _bufferArraySet.offsetArray.data());

      // Set the type of primitive that should be rendered from this vertex buffer.
      deviceContext->IASetPrimitiveTopology(rendererData.topology);

      if (rendererData.indexBufferAccessor != nullptr) {
        // Set the index buffer to active in the input assembler so it can be rendered.
        const auto indexAccessor = rendererData.indexBufferAccessor.get();
        deviceContext->IASetIndexBuffer(
            indexAccessor->buffer->d3dBuffer, rendererData.indexFormat, indexAccessor->offset);
      }
    }
  }

  void drawRendererData(
      const Render_pass::Camera_data& cameraData,
      const SSE::Matrix4& worldTransform,
      Renderer_data& rendererData,
      Render_pass_blend_mode blendMode,
      const Render_light_data& renderLightData,
      std::shared_ptr<Material> material,
      const Mesh_vertex_layout& meshVertexLayout,
      Material_context& materialContext,
      Renderer_animation_data& rendererAnimationData,
      bool wireFrame) override {
    if (rendererData.failedRendering || rendererData.vertexBuffers.empty())
      return;

    auto& commonStates = _deviceRepository->commonStates();

    auto& d3DDeviceResources = deviceResources();
    auto* const context = d3DDeviceResources.GetD3DDeviceContext();

    // Set the rasteriser state
    if (wireFrame)
      context->RSSetState(commonStates.Wireframe());
    else if (material->faceCullMode() == Material_face_cull_mode::Back_Face)
      context->RSSetState(commonStates.CullClockwise());
    else if (material->faceCullMode() == Material_face_cull_mode::Front_Face)
      context->RSSetState(commonStates.CullCounterClockwise());
    else if (material->faceCullMode() == Material_face_cull_mode::None)
      context->RSSetState(commonStates.CullNone());

    // Render the triangles
    auto& materialManager = _scene.manager<IMaterial_manager>();
    try {

      // Make sure that material has is up to date, so that materialManager can simply call
      // `ensureCompilerPropertiesHash` instead of non-const `calculateCompilerPropertiesHash`
      material->calculateCompilerPropertiesHash();

      // What accessor types does this rendererData have?
      materialManager.bind(
          materialContext,
          material,
          meshVertexLayout,
          &renderLightData,
          blendMode,
          cameraData.enablePixelShader);

      loadRendererDataToDeviceContext(rendererData, materialContext);

      materialManager.render(rendererData, worldTransform, rendererAnimationData, cameraData);

      materialManager.unbind();
    } catch (std::exception& ex) {
      rendererData.failedRendering = true;
      LOG(FATAL) << "Failed to drawRendererData, marking failedRendering to true. (" << ex.what()
                 << ")";
    }
  }

  void createDeviceDependentResources() override {
    auto* const device = deviceResources().GetD3DDevice();

    // Material specific rendering properties
    _renderLightData_unlit =
        std::make_unique<decltype(_renderLightData_unlit)::element_type>(device);
    _renderLightData_lit = std::make_unique<decltype(_renderLightData_lit)::element_type>(device);
  }

  std::shared_ptr<D3D_buffer> createBufferFromData(
      const std::string& bufferName,
      const uint8_t* data,
      size_t dataSize,
      UINT bindFlags) const {
    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = static_cast<UINT>(dataSize);
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    // Fill in the sub-resource data.
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = data;
    initData.SysMemPitch = 0;
    initData.SysMemSlicePitch = 0;

    // Create the buffer
    ID3D11Buffer* d3dBuffer;
    ThrowIfFailed(
        deviceResources().GetD3DDevice()->CreateBuffer(&bufferDesc, &initData, &d3dBuffer));

    DX::ThrowIfFailed(d3dBuffer->SetPrivateData(
        WKPDID_D3DDebugObjectName, static_cast<UINT>(bufferName.size()), bufferName.c_str()));

    return std::make_shared<D3D_buffer>(d3dBuffer, dataSize);
  }

  void destroyDeviceDependentResources() override {
    _renderLightData_unlit.reset();
    _renderLightData_lit.reset();
    _createdRendererData.clear();
  }

  void updateLightBuffers() override {
    _renderLightData_lit->updateBuffer(deviceResources().GetD3DDeviceContext());
  }

  std::shared_ptr<D3D_buffer> createBufferFromData(
      const std::string& bufferName,
      const Mesh_buffer& buffer,
      UINT bindFlags) const {
    return createBufferFromData(bufferName, buffer.data, buffer.dataSize, bindFlags);
  }

  std::shared_ptr<Renderer_data> Entity_render_manager::createRendererData(
      std::shared_ptr<Mesh_data> meshData,
      const std::vector<Vertex_attribute_element>& vertexAttributes,
      const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) const {
    auto rendererData = std::make_shared<Renderer_data>();

    switch (meshData->m_meshIndexType) {
    case Mesh_index_type::Triangles:
      rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
      break;
    case Mesh_index_type::Lines:
      rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
      break;
    default:
      OE_THROW(std::exception("Unsupported mesh topology"));
    }

    createMissingVertexAttributes(meshData, vertexAttributes, vertexMorphAttributes);

    // Create D3D Index Buffer
    if (meshData->indexBufferAccessor) {
      rendererData->indexCount = meshData->indexBufferAccessor->count;

      rendererData->indexBufferAccessor = std::make_unique<D3D_buffer_accessor>(
          createBufferFromData(
              "Mesh data index buffer",
              *meshData->indexBufferAccessor->buffer,
              D3D11_BIND_INDEX_BUFFER),
          meshData->indexBufferAccessor->stride,
          meshData->indexBufferAccessor->offset);

      rendererData->indexFormat =
          mesh_utils::getDxgiFormat(Element_type::Scalar, meshData->indexBufferAccessor->component);

      const auto name("Index Buffer (count: " + std::to_string(rendererData->indexCount) + ")");
      rendererData->indexBufferAccessor->buffer->d3dBuffer->SetPrivateData(
          WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
    } else {
      // TODO: Simply log a warning, or try to draw a non-indexed mesh
      rendererData->indexCount = 0;
      OE_THROW(std::runtime_error("CreateRendererData: Missing index buffer"));
    }

    // Create D3D vertex buffers
    rendererData->vertexCount = meshData->getVertexCount();
    for (auto vertexAttrElement : vertexAttributes) {
      const auto& vertexAttr = vertexAttrElement.semantic;
      Mesh_vertex_buffer_accessor* meshAccessor;
      auto vbAccessorPos = meshData->vertexBufferAccessors.find(vertexAttr);
      if (vbAccessorPos != meshData->vertexBufferAccessors.end()) {
        meshAccessor = vbAccessorPos->second.get();
      } else {
        // Since there is no guarantee that the order is the same in the requested attributes array,
        // and the mesh data, we need to do a search to find out the morph target index of this
        // attribute/semantic.
        auto vertexMorphAttributesIdx = -1;
        auto morphTargetIdx = -1;
        for (size_t idx = 0; idx < vertexMorphAttributes.size(); ++idx) {
          const auto& vertexMorphAttribute = vertexMorphAttributes.at(idx);
          if (vertexMorphAttribute.attribute == vertexAttr.attribute) {
            ++morphTargetIdx;

            if (vertexMorphAttribute.semanticIndex == vertexAttr.semanticIndex) {
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
          if (meshData->vertexLayout.morphTargetLayout()[idx].attribute == vertexAttr.attribute) {
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
          OE_THROW(std::runtime_error(string_format(
              "CreateRendererData: Failed to read morph target "
              "%" PRIi32 " for vertex attribute: %s",
              morphTargetIdx,
              Vertex_attribute_meta::vsInputName(vertexAttr))));
        }
        if (meshData->attributeMorphBufferAccessors.at(morphTargetIdx).size() >=
            static_cast<size_t>(morphTargetIdx)) {
          OE_THROW(std::runtime_error(string_format(
              "CreateRendererData: Failed to read morph target "
              "%" PRIi32 " layout offset %" PRIi32 "for vertex attribute: %s",
              morphTargetIdx,
              morphLayoutOffset,
              Vertex_attribute_meta::vsInputName(vertexAttr))));
        }

        meshAccessor =
            meshData->attributeMorphBufferAccessors[morphTargetIdx][morphLayoutOffset].get();
      }

      auto d3DAccessor = std::make_unique<D3D_buffer_accessor>(
          createBufferFromData(
              "Mesh data vertex buffer", *meshAccessor->buffer, D3D11_BIND_VERTEX_BUFFER),
          meshAccessor->stride,
          meshAccessor->offset);

      if (rendererData->vertexBuffers.find(vertexAttr) != rendererData->vertexBuffers.end()) {
        OE_THROW(std::runtime_error(
            std::string("Mesh data contains vertex attribute ") +
            Vertex_attribute_meta::vsInputName(vertexAttr) + " more than once."));
      }
      rendererData->vertexBuffers[vertexAttr] = std::move(d3DAccessor);
    }

    return rendererData;
  }

 private:
  struct Buffer_array_set {
    std::vector<ID3D11Buffer*> bufferArray;
    std::vector<uint32_t> strideArray;
    std::vector<uint32_t> offsetArray;
  };

  Buffer_array_set _bufferArraySet = {};
  std::shared_ptr<D3D_device_repository> _deviceRepository;
  std::vector<std::shared_ptr<Renderer_data>> _createdRendererData;
};
} // namespace oe::internal

template <>
oe::IEntity_render_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<IMaterial_repository>& materialRepository,
    std::shared_ptr<oe::internal::D3D_device_repository>& deviceRepository) {
  return new oe::internal::D3D_entity_render_manager(scene, materialRepository, deviceRepository);
}