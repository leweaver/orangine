#include "pch.h"
#include "Input_manager.h"

#include "DeviceResources.h"
#include <Mouse.h>

using namespace oe;

// DirectX Singletons
Input_manager::Input_manager(Scene& scene, DX::DeviceResources& deviceResources)
	: Manager_base(scene)
{
}

void Input_manager::initialize()
{
	_mouse = std::make_unique<DirectX::Mouse>();
}

void Input_manager::shutdown()
{
	_mouse.reset();
}

void Input_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND window, int /*width*/, int /*height*/)
{
	_mouse->SetWindow(window);
}

void Input_manager::destroyWindowSizeDependentResources()
{
	_mouse = std::make_unique<DirectX::Mouse>();
}
