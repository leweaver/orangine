#include "pch.h"
#include "Scene.h"
#include "User_interface_manager.h"

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_win32.h"
#include "imgui/examples/imgui_impl_dx11.h"

oe::User_interface_manager::User_interface_manager(Scene & scene) 
    : IUser_interface_manager(scene)
{
}

void oe::User_interface_manager::initialize()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().FontGlobalScale = _uiScale;
}

void oe::User_interface_manager::shutdown()
{
    ImGui::DestroyContext();
}

void oe::User_interface_manager::createWindowSizeDependentResources(DX::DeviceResources&, HWND window, int, int)
{
    _window = window;
    const auto& deviceResources = _scene.manager<ID3D_resources_manager>().deviceResources();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(deviceResources.GetD3DDevice(), deviceResources.GetD3DDeviceContext());
    ImGui::StyleColorsDark();    
}

void oe::User_interface_manager::destroyWindowSizeDependentResources()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplDX11_Shutdown();
    _window = nullptr;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool oe::User_interface_manager::processMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_window && ImGui_ImplWin32_WndProcHandler(_window, message, wParam, lParam)) {
        return true;
    }
    return false;
}

void oe::User_interface_manager::render()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    _scene.manager<IEntity_scripting_manager>().renderImGui();
    _scene.manager<IDev_tools_manager>().renderImGui();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool oe::User_interface_manager::keyboardCaptured()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool oe::User_interface_manager::mouseCaptured()
{
    return ImGui::GetIO().WantCaptureMouse;
}
