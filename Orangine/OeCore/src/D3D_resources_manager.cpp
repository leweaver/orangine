﻿#include "pch.h"

#include "D3D_resources_manager.h"
#include "CommonStates.h"

using namespace DirectX;
using namespace oe;
using namespace internal;

std::string D3D_resources_manager::_name = "D3D_resources_manager";

template<>
ID3D_resources_manager* oe::create_manager(Scene & scene, DX::DeviceResources& deviceResources)
{
    return new D3D_resources_manager(scene, deviceResources);
}

D3D_resources_manager::D3D_resources_manager(Scene& scene, DX::DeviceResources& deviceResources)
	: ID3D_resources_manager(scene)
	, _deviceResources(deviceResources)
    , _commonStates(nullptr)
    , _window(nullptr)
{	
}

const std::string& D3D_resources_manager::name() const
{
    return _name;
}

void D3D_resources_manager::createDeviceDependentResources(DX::DeviceResources& deviceResources)
{
	_commonStates = std::make_unique<CommonStates>(_deviceResources.GetD3DDevice());
}

void D3D_resources_manager::destroyDeviceDependentResources()
{
	_commonStates.reset();
}