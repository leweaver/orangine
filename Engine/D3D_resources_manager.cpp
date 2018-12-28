#include "pch.h"

#include "D3D_resources_manager.h"
#include "CommonStates.h"

using namespace oe;
using namespace DirectX;

D3D_resources_manager::D3D_resources_manager(Scene& scene, DX::DeviceResources& deviceResources)
	: ID3D_resources_manager(scene)
	, _deviceResources(deviceResources)
{	
}

void D3D_resources_manager::createDeviceDependentResources(DX::DeviceResources& deviceResources)
{
	_commonStates = std::make_unique<CommonStates>(_deviceResources.GetD3DDevice());
}

void D3D_resources_manager::destroyDeviceDependentResources()
{
	_commonStates.reset();
}
