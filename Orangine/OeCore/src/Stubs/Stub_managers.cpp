#include "pch.h"

#include "../Entity_render_manager.h"
#include "../Material_manager.h"
#include "OeCore/IDevice_resources.h"

#include "OeCore/ITexture_manager.h"
#include "OeCore/IUser_interface_manager.h"
#include "OeCore/Render_pass_generic.h"
#include "OeCore/Render_step_manager.h"

namespace oe {

class Stub_texture final : public Texture {};
class Stub_shadow_map_texture_pool final : public Shadow_map_texture_pool {
 public:
  void createDeviceDependentResources() override {}
  void destroyDeviceDependentResources() override {}
  std::shared_ptr<Texture> borrowTexture() override { return std::make_shared<Stub_texture>(); }
  void returnTexture(std::shared_ptr<Texture> shadowMap) override {}
  std::shared_ptr<Texture> shadowMapDepthTextureArray() override {
    return std::make_shared<Stub_texture>();
  }
  std::shared_ptr<Texture> shadowMapStencilTextureArray() override {
    return std::make_shared<Stub_texture>();
  }
};

class Stub_texture_manager final : public ITexture_manager {
 public:
  explicit Stub_texture_manager()
      : ITexture_manager(), _name("Stub_texture_manager") {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override {}
  void destroyDeviceDependentResources() override {}

  // ITexture_manager implementation
  std::shared_ptr<Texture> createTextureFromBuffer(
      uint32_t stride,
      uint32_t buffer_size,
      std::unique_ptr<uint8_t>& buffer) override {
    return std::make_shared<Stub_texture>();
  }
  std::shared_ptr<Texture> createTextureFromFile(const std::string& fileName) override {
    return std::make_shared<Stub_texture>();
  }
  std::shared_ptr<Texture> createTextureFromFile(
      const std::string& fileName,
      const Sampler_descriptor& samplerDescriptor) override {
    return std::make_shared<Stub_texture>();
  }

  std::shared_ptr<Texture> createDepthTexture() override {
    return std::make_shared<Stub_texture>();
  }
  std::shared_ptr<Texture> createRenderTargetTexture(int width, int height) override {
    return std::make_shared<Stub_texture>();
  }
  std::shared_ptr<Texture> createRenderTargetViewTexture() override {
    return std::make_shared<Stub_texture>();
  }

  std::unique_ptr<Shadow_map_texture_pool> createShadowMapTexturePool(uint32_t a, uint32_t b) override {
    return std::make_unique<Stub_shadow_map_texture_pool>();
  }

  void load(Texture& texture) override {}
  void unload(Texture& texture) override {}

 private:
  std::string _name;
};

class Stub_render_step_manager final : public Render_step_manager {
 public:
  Stub_render_step_manager(
          IScene_graph_manager& sceneGraphManager, IDev_tools_manager& devToolsManager,
          ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
          IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager)
      : Render_step_manager(sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager, lightingManager)
  {}

  // Base class overrides
  void shutdown() override {}
  Viewport getScreenViewport() const override { return {0, 0, 1, 1, 0, 1}; }

  // IRender_step_manager implementation
  void clearRenderTargetView(const Color& color) override {}
  void clearDepthStencil(float f, uint8_t a) override {}
  std::unique_ptr<Render_pass> createShadowMapRenderPass() override {
    return std::make_unique<Render_pass_generic>([](const Camera_data&, const Render_pass&) {});
  }
  void beginRenderNamedEvent(const wchar_t* name) override {}
  void endRenderNamedEvent() override {}
  void createRenderStepResources() override {}
  void destroyRenderStepResources() override {}
  void renderSteps(const Camera_data& cameraData) override {}
};

class Stub_material_context final : public Material_context {
 public:
  void reset() override {}
};

class Stub_material_manager : public Material_manager {
 public:
  Stub_material_manager(IAsset_manager& assetManager) : Material_manager(assetManager) {}
  ~Stub_material_manager() override = default;

  Stub_material_manager(const Stub_material_manager& other) = delete;
  Stub_material_manager(Stub_material_manager&& other) = delete;
  void operator=(const Stub_material_manager& other) = delete;
  void operator=(Stub_material_manager&& other) = delete;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override {}
  void destroyDeviceDependentResources() override { _materialContexts.clear(); }

  // IMaterial_manager implementation
  std::weak_ptr<Material_context> createMaterialContext() override {
    _materialContexts.push_back(std::make_shared<Stub_material_context>());
    return _materialContexts.back();
  }

  void createVertexShader(
      bool enableOptimizations,
      const Material& material,
      Material_context& materialContext) const override {}
  void createPixelShader(
      bool enableOptimizations,
      const Material& material,
      Material_context& materialContext) const override {}
  void createMaterialConstants(const Material& material) override {}
  void loadShaderResourcesToContext(
      const Material::Shader_resources& shaderResources,
      Material_context& materialContext) override {}

  void bindLightDataToDevice(const Render_light_data* renderLightData) override {}
  void bindMaterialContextToDevice(const Material_context& materialContext, bool enablePixelShader)
      override {}
  void render(
      const Renderer_data& rendererData,
      const SSE::Matrix4& worldMatrix,
      const Renderer_animation_data& rendererAnimationData,
      const Camera_data& camera) override {}
  void unbind() override {}

  void updateLightBuffers() override {}

 private:
  // The template arguments here must match the size of the lights array in the shader constant
  // buffer files.

  std::vector<std::shared_ptr<Stub_material_context>> _materialContexts;
};

struct Renderer_data {};

class Stub_entity_render_manager : public oe::internal::Entity_render_manager {
 public:
  Stub_entity_render_manager(ITexture_manager& textureManager, IMaterial_manager& materialManager, ILighting_manager& lightingManager)
      : Entity_render_manager(textureManager, materialManager, lightingManager) {}

  void loadRendererDataToDeviceContext(
      const Renderer_data& rendererData,
      const Material_context& context) override {}

  void drawRendererData(
      const Camera_data& cameraData,
      const SSE::Matrix4& worldTransform,
      Renderer_data& rendererData,
      Render_pass_blend_mode blendMode,
      const Render_light_data& renderLightData,
      std::shared_ptr<Material> material,
      const Mesh_vertex_layout& meshVertexLayout,
      Material_context& materialContext,
      Renderer_animation_data& rendererAnimationData,
      bool wireFrame) override {}

  void createDeviceDependentResources() override {}

  void destroyDeviceDependentResources() override { _createdRendererData.clear(); }

  std::shared_ptr<Renderer_data> createRendererData(
          std::shared_ptr<Mesh_data> meshData,
          const std::vector<Vertex_attribute_element>& vertexAttributes,
          const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) override {
    _createdRendererData.push_back(std::make_shared<Renderer_data>());
    return _createdRendererData.back();
  }

 private:
  std::vector<std::shared_ptr<oe::Renderer_data>> _createdRendererData;
};

class Stub_user_interface_manager final : public IUser_interface_manager {
 public:
  explicit Stub_user_interface_manager()
      : IUser_interface_manager(), _name("Stub_user_interface_manager") {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // Manager_windowDependent implementation
  void createWindowSizeDependentResources(HWND window, int width, int height) override {}
  void destroyWindowSizeDependentResources() override {}

  // Manager_windowsMessageProcessor implementation
  bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override { return false; }

  // IUser_interface_manager implementation
  void render() override {}
  bool keyboardCaptured() override { return false; }
  bool mouseCaptured() override { return false; }
  void preInit_setUIScale(float uiScale) override {}

 private:
  std::string _name;
};

template <>
void create_manager(Manager_instance<IEntity_render_manager>& out, ITexture_manager& textureManager, IMaterial_manager& materialManager, ILighting_manager& lightingManager) {
  out = Manager_instance<IEntity_render_manager>(std::make_unique<Stub_entity_render_manager>(textureManager, materialManager, lightingManager));
}
//
//template<>
//void create_manager(
//        Manager_instance<IRender_step_manager>& out, IScene_graph_manager& sceneGraphManager,
//        IDev_tools_manager& devToolsManager, ITexture_manager& textureManager, IShadowmap_manager& shadowmapManager,
//        IEntity_render_manager& entityRenderManager, ILighting_manager& lightingManager)
//{
//  out = Manager_instance<IRender_step_manager>(std::make_unique<Stub_render_step_manager>(
//          sceneGraphManager, devToolsManager, textureManager, shadowmapManager, entityRenderManager, lightingManager));
//}

template <> void create_manager(Manager_instance<IUser_interface_manager>& out, IDevice_resources& dr) {
  out = Manager_instance<IUser_interface_manager>(std::make_unique<Stub_user_interface_manager>());
}

template <> void create_manager(Manager_instance<IMaterial_manager>& out, IAsset_manager& assetManager) {
  out = Manager_instance<IMaterial_manager>(std::make_unique<Stub_material_manager>(assetManager));
}

template <> void create_manager(Manager_instance<ITexture_manager>& out, IDevice_resources& dr) {
  out = Manager_instance<ITexture_manager>(std::make_unique<Stub_texture_manager>());
}

} // namespace oe
