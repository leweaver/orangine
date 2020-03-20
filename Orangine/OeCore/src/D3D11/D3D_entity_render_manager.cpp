#include "pch.h"

#include "../Entity_render_manager.h"

#include "OeCore/DeviceResources.h"
#include "OeCore/Scene.h"

#include "CommonStates.h"
#include "Device_repository.h"
#include "DirectX_utils.h"

using namespace DirectX;

namespace oe::internal {
class D3D_entity_render_manager : public oe::internal::Entity_render_manager {
 public:
  D3D_entity_render_manager(
      Scene& scene,
      std::shared_ptr<IMaterial_repository> materialRepository,
      std::shared_ptr<Device_repository> deviceRepository)
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

      for (const auto vsInput : context.compiledMaterial->vsInputs) {
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
    const auto context = d3DDeviceResources.GetD3DDeviceContext();

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
          renderLightData,
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

  void loadTexture(Texture& texture) override {
    texture.load(_deviceRepository->deviceResources().GetD3DDevice());
  }

  void createDeviceDependentResources() override {
    auto device = deviceResources().GetD3DDevice();

    // Material specific rendering properties
    _renderLightData_unlit =
        std::make_unique<decltype(_renderLightData_unlit)::element_type>(device);
    _renderLightData_lit = std::make_unique<decltype(_renderLightData_lit)::element_type>(device);
  }

  std::shared_ptr<D3D_buffer> createBufferFromData(
      const std::string& bufferName,
      const uint8_t* data,
      size_t dataSize,
      UINT bindFlags) const override {
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
  }
  void updateLightBuffers() override {
    _renderLightData_lit->updateBuffer(deviceResources().GetD3DDeviceContext());
  }

 private:
  struct Buffer_array_set {
    std::vector<ID3D11Buffer*> bufferArray;
    std::vector<uint32_t> strideArray;
    std::vector<uint32_t> offsetArray;
  };

  Buffer_array_set _bufferArraySet = {};
  std::shared_ptr<Device_repository> _deviceRepository;
};
} // namespace oe::internal

template <>
oe::IEntity_render_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<IMaterial_repository>& materialRepository,
    std::shared_ptr<oe::internal::Device_repository>& deviceRepository) {
  return new oe::internal::D3D_entity_render_manager(scene, materialRepository, deviceRepository);
}