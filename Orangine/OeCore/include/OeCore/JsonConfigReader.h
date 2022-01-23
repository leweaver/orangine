#pragma once

#include <OeCore/IConfigReader.h>

#include <string>
#include <json.hpp>

namespace oe {
class JsonConfigReader : public IConfigReader {
 public:
  explicit JsonConfigReader(const std::wstring& configFileName);

  std::wstring readPath(const std::string& configPath) const override;
  std::unordered_map<std::wstring, std::wstring> readPathToPathMap(
      const std::string& jsonPath) const override;

 private:
  nlohmann::json::value_type getJsonValue(const std::string& jsonPath) const;
  nlohmann::json::value_type _configJson;
};
} // namespace oe