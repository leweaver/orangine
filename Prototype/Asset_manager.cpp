#include "pch.h"
#include "Asset_manager.h"

using namespace oe;

Asset_manager::Asset_manager(Scene& scene)
	: Manager_base(scene)
{
}

void Asset_manager::initialize()
{
	_assets[0LL] = { 
		0,
		L"data/models/Cube.gltf"
		};
}

void Asset_manager::tick()
{
}

bool Asset_manager::getFilePath(FAsset_id assetId, std::wstring& path) const
{
	const auto pos = _assets.find(assetId);
	if (pos != _assets.end()) {  // NOLINT
		path = pos->second.filename;
		return true;
	}
	else {
		return false;
	}
}