#pragma once

#include <OeCore/IConfigReader.h>

#include <string>
#include <unordered_map>

namespace oe {
namespace internal {
class Yaml_file_processor;
}

class Yaml_config_reader : public IConfigReader {
 public:
  explicit Yaml_config_reader(const std::wstring& configFileName);
  ~Yaml_config_reader();

  std::wstring readPath(const std::string& configPath) const override;
  std::unordered_map<std::wstring, std::wstring> readPathToPathMap(
          const std::string& jsonPath) const override;

 private:
  internal::Yaml_file_processor* _fileProcessor;
};
} // namespace oe