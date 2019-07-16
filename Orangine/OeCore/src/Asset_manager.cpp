﻿#include "pch.h"
#include "Asset_manager.h"
#include <OeCore/EngineUtils.h>
#include <OeCore/IConfigReader.h>

using namespace oe;
using namespace internal;

std::string Asset_manager::_name = "Asset_manager";

template<>
IAsset_manager* oe::create_manager(Scene& scene)
{
    return new Asset_manager(scene);
}

Asset_manager::Asset_manager(Scene& scene)
	: IAsset_manager(scene)
{
}

void Asset_manager::initialize()
{
	_assets[0LL] = { 
		0,
		L"data/models/Cube.gltf"
		};

	_initialized = true;
}

void Asset_manager::shutdown()
{
}

const std::string& Asset_manager::name() const
{
    return _name;
}

void oe::internal::Asset_manager::setDataPath(const std::wstring& dataPath)
{
	if (_initialized) {
		throw std::logic_error("Cannot change data path after Asset_manager is initialized.");
	}
	_dataPath = dataPath;
}

void Asset_manager::setDataPathOverrides(std::map<std::wstring, std::wstring> &&paths) {
    _dataPathOverrides = move(paths);
}

const std::map<std::wstring, std::wstring>&
Asset_manager::dataPathOverrides(std::map<std::wstring, std::wstring>&& paths) const
{
    return _dataPathOverrides;
}

std::wstring Asset_manager::makeAbsoluteAssetPath(const std::wstring &path) const {
    for (const auto& pair : _dataPathOverrides ) {
        if (str_starts(path, pair.first)) {
            return pair.second + L"/" + path;
        }
    }
    return _dataPath + L"/" + path;
}

void Asset_manager::loadConfig(const IConfigReader& configReader) {
    Manager_base::loadConfig(configReader);

	this->setDataPath(
		configReader.readPath("data_path")
	);
	this->setDataPathOverrides(
		configReader.readPathToPathMap("data_path_overrides")
	);
}
