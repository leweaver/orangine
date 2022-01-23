#include "pch.h"

#include <OeCore/JsonConfigReader.h>
#include <OeCore/FileUtils.h>

using namespace oe;

JsonConfigReader::JsonConfigReader(const std::wstring& configFileName) : _configJson("") {
  const auto jsonStr = get_file_contents(configFileName.c_str());
  _configJson = nlohmann::json::parse(jsonStr.begin(), jsonStr.end());
}

nlohmann::json::value_type JsonConfigReader::getJsonValue(const std::string& jsonPath) const {

  std::string::size_type lastPos = 0, pos;
  nlohmann::json::value_type currentElement = _configJson;
  while ((pos = jsonPath.find('.', lastPos)) != std::string::npos) {
    assert(pos > lastPos);

    auto nextChildName = jsonPath.substr(lastPos, pos - lastPos);

    if (!currentElement.is_object()) {
      OE_THROW(std::runtime_error("Invalid config path: " + jsonPath));
    }
    currentElement = currentElement[nextChildName];

    lastPos = pos + 1;
  }

  auto lastChildName = jsonPath.substr(lastPos);
  if (!currentElement.is_object()) {
    OE_THROW(std::runtime_error("Invalid config path: " + jsonPath));
  }
  return currentElement[lastChildName];
}

std::wstring JsonConfigReader::readPath(const std::string& jsonPath) const {
  const auto currentElement = getJsonValue(jsonPath);
  if (!currentElement.is_string())
    OE_THROW(std::runtime_error("Expected configuration " + jsonPath + " to be a string"));

  return utf8_decode(currentElement);
}

std::unordered_map<std::wstring, std::wstring> JsonConfigReader::readPathToPathMap(
    const std::string& jsonPath) const {
  std::unordered_map<std::wstring, std::wstring> result;

  const auto currentElement = getJsonValue(jsonPath);
  if (!currentElement.is_object()) {
    OE_THROW(std::runtime_error("Expected configuration " + jsonPath + " to be an object"));
  }

  for (auto it = currentElement.begin(); it != currentElement.end(); ++it) {
    if (!it.value().is_string()) {
      OE_THROW(std::runtime_error(
          "Expected configuration " + jsonPath +
          " values to be strings. Invalid key: " + it.key()));
    }
    result[utf8_decode(it.key())] = utf8_decode(it.value());
  }
  return result;
}
