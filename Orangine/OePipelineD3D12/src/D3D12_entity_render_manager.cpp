//
// Created by Lewis weaver on 2/25/2022.
//

#include "D3D12_entity_render_manager.h"
#include "D3D12_material_manager.h"
#include "Frame_resources.h"

#include <OeCore/Mesh_utils.h>

using Microsoft::WRL::ComPtr;
using oe::Mesh_gpu_data;
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

void D3D12_entity_render_manager::createDeviceDependentResources()
{
  auto device = _deviceResources.GetD3DDevice();

  _deviceDependent.device = device;
}

void D3D12_entity_render_manager::destroyDeviceDependentResources()
{
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
    rendererData->indexBuffer = Gpu_buffer::createAndUpload(
            _deviceDependent.device, name, *meshData->indexBufferAccessor, _deviceResources.getResourceUploader(),
            D3D12_RESOURCE_STATE_INDEX_BUFFER);
    rendererData->indexFormat =
            mesh_utils::getDxgiFormat(Element_type::Scalar, meshData->indexBufferAccessor->component);
  }
  else {
    // TODO: Simply log a warning, or try to draw a non-indexed mesh
    rendererData->indexCount = 0;
    OE_THROW(std::runtime_error("CreateRendererData: Missing index buffer"));
  }

  // Create D3D vertex buffers
  rendererData->vertexCount = meshData->getVertexCount();
  rendererData->vertexBufferViews.resize(vertexAttributes.size());
  auto viewIter = rendererData->vertexBufferViews.begin();
  for (auto vertexAttrElement : vertexAttributes) {
    const auto& vertexAttr = vertexAttrElement.semantic;
    if (rendererData->vertexBuffers.find(vertexAttr) != rendererData->vertexBuffers.end()) {
      OE_THROW(std::runtime_error(
              std::string("Mesh data contains vertex attribute ") + Vertex_attribute_meta::vsInputName(vertexAttr) +
              " more than once."));
    }

    Mesh_vertex_buffer_accessor* meshAccessor = findAccessorForSemantic(meshData, vertexMorphAttributes, vertexAttr);

    std::stringstream name;
    name << "Vertex buffer";
    if (!meshData->name.empty()) {
      name << " (meshData: " << meshData->name << ")";
    }
    name << " (count: " << std::to_string(rendererData->indexCount) << ")";
    std::shared_ptr<Gpu_buffer_reference> vertexGpuBuffer = getOrCreateUsage(name.str(), meshAccessor->buffer, meshAccessor->stride);
    rendererData->vertexBuffers[vertexAttr] = vertexGpuBuffer;
    *(viewIter++) = vertexGpuBuffer->gpuBuffer->getAsVertexBufferView(static_cast<size_t>(meshAccessor->offset));
  }

  _deviceDependent.createdRendererData.push_back(rendererData);
  return rendererData;
}

std::shared_ptr<Gpu_buffer_reference> D3D12_entity_render_manager::getOrCreateUsage(
        const std::string_view& name, const std::shared_ptr<Mesh_buffer>& meshBuffer, size_t vertexSize)
{
  auto pos = _deviceDependent.meshBufferToGpuBuffers.find(meshBuffer);
  if (pos == _deviceDependent.meshBufferToGpuBuffers.end()) {
    std::wstring name_w;
    if (!name.empty()) {
      name_w = oe::utf8_decode(name);
    }
    auto bufferPtr = Gpu_buffer::create(_deviceDependent.device, name_w, meshBuffer->dataSize, vertexSize);
    bufferPtr->uploadAndTransition(
            _deviceResources.getResourceUploader(), {meshBuffer->data, meshBuffer->dataSize},
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    auto gpuBufferRef = std::make_shared<Gpu_buffer_reference>();
    gpuBufferRef->gpuBuffer = std::move(bufferPtr);
    auto insertRes =
            _deviceDependent.meshBufferToGpuBuffers.insert(std::make_pair(meshBuffer, std::move(gpuBufferRef)));
    OE_CHECK(insertRes.second);
    pos = insertRes.first;
  }
  return pos->second;
}

void D3D12_entity_render_manager::drawRendererData(
        const oe::Camera_data& cameraData, const Matrix4& worldTransform, oe::Mesh_gpu_data& rendererData,
        const Depth_stencil_config& depthStencilConfig, const oe::Render_light_data& renderLightData,
        std::shared_ptr<Material> material, const oe::Mesh_vertex_layout& meshVertexLayout,
        oe::Material_context_handle materialContext, oe::Renderer_animation_data& rendererAnimationData, bool wireframe)
{
  if (rendererData.failedRendering || rendererData.vertexBuffers.empty()) {
    return;
  }

  try {
    // Make sure that material has is up to date, so that materialManager can simply call
    // `ensureCompilerPropertiesHash` instead of non-const `calculateCompilerPropertiesHash`
    material->calculateCompilerPropertiesHash();

    // Compile shaders if need be
    _materialManager.updateMaterialContext(
            materialContext, material, meshVertexLayout, rendererData, &renderLightData, depthStencilConfig,
            cameraData.enablePixelShader, wireframe);

    if (!_materialManager.isMaterialContextDataReady(materialContext)) {
      // Material is not ready for rendering. Might still be loading textures
      return;
    }

    _materialManager.bind(materialContext);
  }
  catch (std::exception& ex) {
    rendererData.failedRendering = true;
    LOG(FATAL) << "drawRendererData: Failed bind material, marking failedRendering to true. (" << ex.what() << ")";
    return;
  }

  const Root_signature_layout& rootSignatureLayout = _deviceResources.getCurrentFrameResources().getCurrentBoundRootSignature();
  ID3D12GraphicsCommandList4* commandList = _deviceResources.GetPipelineCommandList().Get();
  
  // Upload constant buffer data
  if (rootSignatureLayout.constantBufferAllVisParamIndex != Root_signature_layout::kSlotInvalid) {
    const Shader_constant_layout& constantLayout = material->getShaderConstantLayout();

    Material::Update_constant_buffer_inputs cbInputs{
            worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, renderLightData, rendererAnimationData};

    auto descriptorRange = _deviceResources.getSrvHeap().allocateRange(constantLayout.constantBuffers.size());
    auto currentDescriptorHandle = descriptorRange.cpuHandle;
    for (auto constantBufferInfo : constantLayout.constantBuffers) {
      gsl::span<uint8_t> cpuBuffer{};
      D3D12_GPU_VIRTUAL_ADDRESS gpuBufferAddress{};
      if (constantBufferInfo.getUsage() == Shader_constant_buffer_usage::Per_draw) {
        acquireConstantBufferForSize(
                constantBufferInfo.getUsagePerDraw().bufferSizeInBytes, cpuBuffer, gpuBufferAddress);
        material->updatePerDrawConstantBuffer(cpuBuffer, constantBufferInfo, cbInputs);
      }
      else if (constantBufferInfo.getUsage() == Shader_constant_buffer_usage::External_buffer) {
        // TODO: Custom handles.
        // TODO: Skeleton animation data.
        auto handle = constantBufferInfo.getUsageExternalBuffer().handle;
        if (handle == Material::kExternalUsageLightingHandle) {
          acquireConstantBufferForSize(renderLightData.getConstantBufferSize(), cpuBuffer, gpuBufferAddress);
          renderLightData.copyConstantsToBuffer(cpuBuffer);
        }
      }

      D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{gpuBufferAddress, static_cast<UINT>(cpuBuffer.size())};
      _deviceResources.GetD3DDevice()->CreateConstantBufferView(&cbvDesc, currentDescriptorHandle);
      currentDescriptorHandle.Offset(static_cast<int32_t>(descriptorRange.incrementSize));
    }

    // Set our constant buffers descriptor table address
    LOG(DEBUG) << "  Root[" << rootSignatureLayout.constantBufferAllVisParamIndex << "] = CBV Table (size=" << descriptorRange.descriptorCount << ")";
    commandList->SetGraphicsRootDescriptorTable(
            rootSignatureLayout.constantBufferAllVisParamIndex,
            descriptorRange.gpuHandle);

    // Queue the descriptor to be released the next time this frame index is started
    _deviceResources.getSrvHeap().releaseRangeAtFrameStart(descriptorRange, _deviceResources.getCurrentFrameIndex());
  }
  
  try {
    // Set geometry
    const auto numVertexViews = static_cast<uint32_t>(rendererData.vertexBufferViews.size());
    commandList->IASetVertexBuffers(0, numVertexViews, rendererData.vertexBufferViews.data());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw the primitive
    if (rendererData.indexCount > 0) {
      D3D12_INDEX_BUFFER_VIEW indexBufferView = rendererData.indexBuffer->getAsIndexBufferView(rendererData.indexFormat);
      commandList->IASetIndexBuffer(&indexBufferView);
      commandList->DrawIndexedInstanced(rendererData.indexCount, 1, 0, 0, 0);
    }
    else {
      commandList->DrawInstanced(rendererData.vertexCount, 1, 0, 0);
    }
  }
  catch (std::exception& ex) {
    rendererData.failedRendering = true;
    LOG(FATAL) << "drawRendererData: Failed render, marking failedRendering to true. (" << ex.what() << ")";
  }

  _materialManager.unbind();
}

void D3D12_entity_render_manager::acquireConstantBufferForSize(size_t requiredSize, gsl::span<uint8_t>& cpuBuffer, D3D12_GPU_VIRTUAL_ADDRESS& gpuAddress)
{
  Constant_buffer_pool& pool = _deviceResources.getCurrentFrameResources().getOrCreateConstantBufferPoolToFit(
          requiredSize, 1);

  uint8_t* cpuBufferData = nullptr;
  OE_CHECK(pool.getTop(cpuBufferData, gpuAddress) && cpuBufferData);
  pool.pop(1);

  cpuBuffer = {cpuBufferData, pool.getItemSizeInBytes()};
}
