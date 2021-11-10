#include "pch.h"

#include "Asset_manager.h"

#include <OeCore/EngineUtils.h>
#include <OeCore/IConfigReader.h>

using namespace oe;
using namespace internal;

std::string Asset_manager::_name = "Asset_manager";

template<> void oe::create_manager(Manager_instance<IAsset_manager>& out)
{
  out = Manager_instance<IAsset_manager>(std::make_unique<Asset_manager>());
}

Asset_manager::Asset_manager() : IAsset_manager(), Manager_base() {}

void Asset_manager::initialize() { _initialized = true; }

void Asset_manager::shutdown() {}

const std::string& Asset_manager::name() const { return _name; }

void Asset_manager::preInit_setDataPath(const std::wstring& dataPath) {
  if (_initialized) {
    OE_THROW(std::logic_error("Cannot change data path after Asset_manager is initialized."));
  }
  _dataPath = dataPath;
}

void Asset_manager::setDataPathOverrides(std::map<std::wstring, std::wstring>&& paths) {
  _dataPathOverrides = move(paths);
}

const std::map<std::wstring, std::wstring>& Asset_manager::dataPathOverrides(
    std::map<std::wstring, std::wstring>&& paths) const {
  return _dataPathOverrides;
}

std::wstring Asset_manager::makeAbsoluteAssetPath(const std::wstring& path) const {
  for (const auto& pair : _dataPathOverrides) {
    if (str_starts(path, pair.first)) {
      return pair.second + L"/" + path;
    }
  }
  if (_fallbackDataPathAllowed && !_dataPathOverrides.empty()) {
    return _dataPath + L"/" + path;
  }
  OE_THROW(std::invalid_argument(
      "Given data path does not match any data path overrides, and fallback data path is "
      "forbidden: " +
      utf8_encode(path)));
}

void Asset_manager::loadConfig(const IConfigReader& configReader) {
  Manager_base::loadConfig(configReader);

  this->preInit_setDataPath(configReader.readPath("data_path"));
  this->setDataPathOverrides(configReader.readPathToPathMap("data_path_overrides"));
}
