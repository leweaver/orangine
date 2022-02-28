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

class Stub_material_context final : public Material_context {
 public:
  void releaseResources() override {}
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


template <> void create_manager(Manager_instance<IUser_interface_manager>& out, IDevice_resources& dr) {
  out = Manager_instance<IUser_interface_manager>(std::make_unique<Stub_user_interface_manager>());
}

template <> void create_manager(Manager_instance<ITexture_manager>& out, IDevice_resources& dr) {
  out = Manager_instance<ITexture_manager>(std::make_unique<Stub_texture_manager>());
}

} // namespace oe
