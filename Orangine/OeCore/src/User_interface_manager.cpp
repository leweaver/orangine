#include "pch.h"

#include "User_interface_manager.h"

#include "OeCore/Scene.h"

#include <OeThirdParty/imgui.h>
#include <OeThirdParty/imgui_impl_dx11.h>
#include <OeThirdParty/imgui_impl_win32.h>

using namespace oe;
using namespace internal;

std::string User_interface_manager::_name = "User_interface_manager";

template <>
IUser_interface_manager* oe::create_manager(
    Scene& scene,
    std::shared_ptr<D3D_device_repository>& device_repository) {
  return new User_interface_manager(scene, device_repository);
}

User_interface_manager::User_interface_manager(
    Scene& scene,
    std::shared_ptr<D3D_device_repository> device_repository)
    : IUser_interface_manager(scene), _deviceRepository(device_repository) {}

void User_interface_manager::initialize() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().FontGlobalScale = _uiScale;
}

void User_interface_manager::shutdown() { ImGui::DestroyContext(); }

const std::string& User_interface_manager::name() const { return _name; }

void User_interface_manager::createWindowSizeDependentResources(HWND window, int, int) {
  _window = window;
  const auto& deviceResources = _deviceRepository->deviceResources();
  ImGui_ImplWin32_Init(window);
  ImGui_ImplDX11_Init(deviceResources.GetD3DDevice(), deviceResources.GetD3DDeviceContext());
  ImGui::StyleColorsDark();
}

void User_interface_manager::destroyWindowSizeDependentResources() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplDX11_Shutdown();
  _window = nullptr;
}

extern LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool User_interface_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam) {
  return _window && ImGui_ImplWin32_WndProcHandler(_window, message, wParam, lParam);
}

void User_interface_manager::render() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  _scene.manager<IScene_graph_manager>().renderImGui();
  _scene.manager<IEntity_scripting_manager>().renderImGui();
  _scene.manager<IDev_tools_manager>().renderImGui();
  _scene.manager<IBehavior_manager>().renderImGui();

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool User_interface_manager::keyboardCaptured() { return ImGui::GetIO().WantCaptureKeyboard; }

bool User_interface_manager::mouseCaptured() { return ImGui::GetIO().WantCaptureMouse; }