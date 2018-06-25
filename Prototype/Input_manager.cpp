#include "pch.h"
#include "Input_manager.h"

#include "DeviceResources.h"

using namespace oe;

Input_manager::Input_manager(Scene& scene, DX::DeviceResources& deviceResources)
	: Manager_base(scene)
{
}

void Input_manager::initialize() {}
void Input_manager::shutdown() {}
