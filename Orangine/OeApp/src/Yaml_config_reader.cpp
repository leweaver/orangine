#include "pch.h"

#include <OeApp/Yaml_config_reader.h>
#include <OeCore/FileUtils.h>

#include <yaml-cpp/yaml.h>

using namespace oe;

namespace oe::internal {
class Yaml_file_processor {
 public:
  explicit Yaml_file_processor(const std::wstring& configFileName) {
    auto fileName_utf8 = utf8_encode(configFileName);
    std::ifstream in(fileName_utf8, std::ios::in | std::ios::binary);
    _root = YAML::Load(in);
  }

  YAML::Node getNode(const std::string& path) {

    std::string::size_type lastPos = 0, pos = 0;
    auto currentElement = _root;
    /*
    while ((pos = path.find('.', lastPos)) != std::string::npos) {
      OE_CHECK(pos > lastPos);

      auto nextChildName = path.substr(lastPos, pos - lastPos);

      if (!currentElement.is_object()) {
        OE_THROW(std::runtime_error("Invalid config path: " + path));
      }
      currentElement = currentElement[nextChildName];

      lastPos = pos + 1;
    }

    auto lastChildName = path.substr(lastPos);
    if (!currentElement.is_object()) {
      OE_THROW(std::runtime_error("Invalid config path: " + path));
    }
    return currentElement[lastChildName];
     */
    return {};
  }


 private:
  YAML::Node _root;
};
}

Yaml_config_reader::Yaml_config_reader(const std::wstring& configFileName)
        : _fileProcessor(new internal::Yaml_file_processor(configFileName))
{
}

Yaml_config_reader::~Yaml_config_reader()
{
  delete _fileProcessor;
}


std::wstring Yaml_config_reader::readPath(const std::string& jsonPath) const
{
  const auto currentElement = _fileProcessor->getNode(jsonPath);
  /*
  if (!currentElement.is_string())
    OE_THROW(std::runtime_error("Expected configuration " + jsonPath + " to be a string"));

  return utf8_decode(currentElement);
   */ return  L"";
}

std::unordered_map<std::wstring, std::wstring> Yaml_config_reader::readPathToPathMap(
        const std::string& jsonPath) const {
  std::unordered_map<std::wstring, std::wstring> result;

  const auto currentElement = _fileProcessor->getNode(jsonPath);
  /*
  if (!currentElement.is_object())
    OE_THROW(std::runtime_error("Expected configuration " + jsonPath + " to be an object"));

  for (auto it = currentElement.begin(); it != currentElement.end(); ++it) {
    if (!it.value().is_string()) {
      OE_THROW(std::runtime_error(
              "Expected configuration " + jsonPath +
              " values to be strings. Invalid key: " + it.key()));
    }
    result[utf8_decode(it.key())] = utf8_decode(it.value());
  }

  return result;
   */
  return {};
}
