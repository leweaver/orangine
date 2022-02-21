#pragma once

#include <OeCore/IAsset_manager.h>

#include <map>
#include <string>

namespace oe::internal {
class Asset_manager : public IAsset_manager, public Manager_base {
 public:
  Asset_manager();

  void preInit_setDataPath(const std::string& dataPath) override;
  const std::string& getDataPath() const override { return _dataPath; }

  void setDataPathOverrides(std::unordered_map<std::string, std::string>&& paths) override;
  const std::unordered_map<std::string, std::string>&
  dataPathOverrides(std::unordered_map<std::string, std::string>&& paths) const override;

  void setFallbackDataPathAllowed(bool allow) override { _fallbackDataPathAllowed = allow; }
  bool fallbackDataPathAllowed() const override { return _fallbackDataPathAllowed; }

  std::string makeAbsoluteAssetPath(const std::string& path) const override;

  // Manager_base implementation
  void loadConfig(const IConfigReader&) override;
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

 private:
  static std::string _name;
  std::string _dataPath = "./data";
  std::unordered_map<std::string, std::string> _dataPathOverrides;
  bool _fallbackDataPathAllowed = true;

  bool _initialized = false;
};
} // namespace oe::internal
