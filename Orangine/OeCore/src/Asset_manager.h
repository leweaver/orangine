#pragma once

#include <OeCore/IAsset_manager.h>

#include <map>
#include <string>

namespace oe::internal {
class Asset_manager : public IAsset_manager, public Manager_base {
 public:
  Asset_manager();

  void preInit_setDataPath(const std::wstring& dataPath) override;
  const std::wstring& getDataPath() const override { return _dataPath; }

  void setDataPathOverrides(std::map<std::wstring, std::wstring>&& paths) override;
  const std::map<std::wstring, std::wstring>&
  dataPathOverrides(std::map<std::wstring, std::wstring>&& paths) const override;

  void setFallbackDataPathAllowed(bool allow) override { _fallbackDataPathAllowed = allow; }
  bool fallbackDataPathAllowed() const override { return _fallbackDataPathAllowed; }

  std::wstring makeAbsoluteAssetPath(const std::wstring& path) const override;

  // Manager_base implementation
  void loadConfig(const IConfigReader&) override;
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

 private:
  static std::string _name;
  std::wstring _dataPath = L"./data";
  std::map<std::wstring, std::wstring> _dataPathOverrides;
  bool _fallbackDataPathAllowed = true;

  bool _initialized = false;
};
} // namespace oe::internal
