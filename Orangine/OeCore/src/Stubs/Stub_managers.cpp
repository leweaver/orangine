#include "OeCore/IDevice_resources.h"

#include "OeCore/ITexture_manager.h"
#include "OeCore/IUser_interface_manager.h"
#include "OeCore/Render_pass_generic.h"
#include "OeCore/Render_step_manager.h"

namespace oe {

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

} // namespace oe
