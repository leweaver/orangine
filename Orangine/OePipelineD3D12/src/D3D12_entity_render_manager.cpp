//
// Created by Lewis weaver on 2/25/2022.
//

#include "D3D12_entity_render_manager.h"

#include <OeCore/Mesh_utils.h>

using oe::Mesh_gpu_data;
using Microsoft::WRL::ComPtr;
using namespace oe::pipeline_d3d12;

template<>
void oe::create_manager(
        Manager_instance<IEntity_render_manager>& out, ITexture_manager& textureManager,
        IMaterial_manager& materialManager, ILighting_manager& lightingManager,
        IPrimitive_mesh_data_factory& primitiveMeshDataFactory, D3D12_device_resources& deviceResources)
{
  out = Manager_instance<IEntity_render_manager>(std::make_unique<D3D12_entity_render_manager>(
          textureManager, materialManager, lightingManager, primitiveMeshDataFactory, deviceResources));
}
void D3D12_entity_render_manager::drawRendererData(
        const oe::Camera_data& cameraData, const Matrix4& worldTransform, oe::Mesh_gpu_data& rendererData,
        oe::Render_pass_blend_mode blendMode, const oe::Render_light_data& renderLightData,
        std::shared_ptr<Material> ptr, const oe::Mesh_vertex_layout& meshVertexLayout,
        oe::Material_context& materialContext, oe::Renderer_animation_data& rendererAnimationData, bool wireframe)
{
}

void D3D12_entity_render_manager::createDeviceDependentResources()
{
  auto device = _deviceResources.GetD3DDevice();

  _deviceDependent.device = device;
}

void D3D12_entity_render_manager::destroyDeviceDependentResources() {
  _deviceDependent = {};
}

std::shared_ptr<Mesh_gpu_data> D3D12_entity_render_manager::createRendererData(
        std::shared_ptr<Mesh_data> meshData, const std::vector<Vertex_attribute_element>& vertexAttributes,
        const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes)
{
  // This method creates the vertex and index buffers, and schedules upload to GPU. The materialManager is what will
  // create the pipeline state object.
  OE_CHECK(_deviceDependent.device);

  auto rendererData = std::make_shared<Mesh_gpu_data>();

  createMissingVertexAttributes(meshData, vertexAttributes, vertexMorphAttributes);

  // Create D3D Index Buffer
  if (meshData->indexBufferAccessor) {
    rendererData->indexCount = meshData->indexBufferAccessor->count;

    const auto name = oe::utf8_decode("Index Buffer (count: " + std::to_string(rendererData->indexCount) + ")");
    rendererData->indexBuffer = Gpu_buffer::create(_deviceDependent.device, name, *meshData->indexBufferAccessor, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    rendererData->indexFormat = mesh_utils::getDxgiFormat(Element_type::Scalar, meshData->indexBufferAccessor->component);

  } else {
    // TODO: Simply log a warning, or try to draw a non-indexed mesh
    rendererData->indexCount = 0;
    OE_THROW(std::runtime_error("CreateRendererData: Missing index buffer"));
  }

  // Create D3D vertex buffers
  rendererData->vertexCount = meshData->getVertexCount();
  for (auto vertexAttrElement : vertexAttributes) {
    const auto& vertexAttr = vertexAttrElement.semantic;
    if (rendererData->vertexBuffers.find(vertexAttr) != rendererData->vertexBuffers.end()) {
      OE_THROW(std::runtime_error(
              std::string("Mesh data contains vertex attribute ") +
              Vertex_attribute_meta::vsInputName(vertexAttr) + " more than once."));
    }

    Mesh_vertex_buffer_accessor* meshAccessor = findAccessorForSemantic(meshData, vertexMorphAttributes, vertexAttr);

    const auto name = oe::utf8_decode(
            "Vertex Buffer " + vertexAttributeToString(vertexAttr.attribute) +
            std::to_string(vertexAttr.semanticIndex) + " (count: " + std::to_string(rendererData->indexCount) + ")");
    rendererData->vertexBuffers[vertexAttr] = Gpu_buffer::create(_deviceDependent.device, name, *meshAccessor);
  }

  _deviceDependent.createdRendererData.push_back(rendererData);
  return rendererData;
}
