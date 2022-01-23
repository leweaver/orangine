#pragma once

#include <string>
#include <unordered_map>

namespace oe {
class IConfigReader {
 public:
  virtual ~IConfigReader() = default;

  virtual std::wstring readPath(const std::string& jsonPath) const = 0;
  virtual std::unordered_map<std::wstring, std::wstring> readPathToPathMap(const std::string& jsonPath) const = 0;
};

}// namespace oe