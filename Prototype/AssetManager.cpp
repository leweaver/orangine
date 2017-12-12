#include "pch.h"
#include "AssetManager.h"

using namespace OE;

AssetManager::AssetManager(Scene &scene)
	: ManagerBase(scene)
{
}

void AssetManager::Initialize()
{
	AssetInfo info;
	info.m_id = 0;
	info.m_filename = "data/models/Cube.gltf";
	m_assets[0] = info;
}

void AssetManager::Tick()
{
}

bool AssetManager::GetFilePath(FAssetId assetId, std::string &path) const
{
	const auto pos = m_assets.find(assetId);
	if (pos == m_assets.end())
		return false;
	path = pos->second.m_filename;
	return true;
}