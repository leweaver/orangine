#include <OeApp/Yaml_config_reader.h>
#include <OeCore/FileUtils.h>

#include <yaml-cpp/yaml.h>

#include <gsl/span>

namespace oe::app {

namespace internal {
class Yaml_file_processor {
 public:
  static constexpr std::array<const char*, 5> kNodeTypeNames = {{
          "Undefined", "Null", "Scalar", "Sequence", "Map"
  }};

  static void checkElementType(const std::string& configPath, const YAML::Node& currentElement, YAML::NodeType::value expectedType)
  {
    OE_CHECK_FMT(
            currentElement.Type() == expectedType, "%s must be a %s, but is a %s", configPath.c_str(),
            kNodeTypeNames.at(size_t(expectedType)),
            kNodeTypeNames.at(size_t(currentElement.Type())));
  }

  explicit Yaml_file_processor(const std::wstring& configFileName)
  {
    auto fileName_utf8 = utf8_encode(configFileName);
    std::ifstream in(fileName_utf8, std::ios::in | std::ios::binary);
    _root = YAML::Load(in);
  }

  explicit Yaml_file_processor(std::iostream& config)
  {
    _root = YAML::Load(config);
  }

  bool getChildNode(const std::string& key, const YAML::Node& parent, YAML::Node& child) const
  {
    if (parent.IsMap()) {
      auto foundNode = parent[key];
      if (!foundNode.IsDefined()) {
        return false;
      }
      child.reset(foundNode);
      return true;
    }
    else if (parent.IsSequence() || parent.size()) {
      uint64_t index{};
      auto ss = std::istringstream(key) >> index;
      if (ss.fail()) {
        LOG(WARNING) << "Failed to get config node; sequence index '" << key << "' cannot be converted to int.";
        return false;
      }
      if (index >= parent.size()) {
        return false;
      }
      child.reset(parent[index]);
      return true;
    }
    return false;
  }

  bool getNode(const std::string& path, YAML::Node& node) const
  {
    std::string::size_type lastPos = 0, pos = 0;
    YAML::Node currentElement;
    currentElement.reset(_root);

    while ((pos = path.find('.', lastPos)) != std::string::npos) {
      OE_CHECK(pos > lastPos);

      auto nextChildName = path.substr(lastPos, pos - lastPos);
      if (!getChildNode(nextChildName, currentElement, currentElement)) {
        auto defaultPos = _defaults.find(path.substr(0, pos));
        if (defaultPos == _defaults.end()) {
          return false;
        }
        currentElement.reset(defaultPos->second);
      }

      lastPos = pos + 1;
    }

    auto lastChildName = path.substr(lastPos);
    if (!getChildNode(lastChildName, currentElement, node)) {
      auto defaultPos = _defaults.find(path.substr(0, pos));
      if (defaultPos == _defaults.end()) {
        return false;
      }
      node.reset(defaultPos->second);
    }
    return true;
  }

  template<typename TValue> void setDefault(const std::string& configPath, TValue value)
  {
    YAML::Node& node = _defaults[configPath];
    node = value;
  }

  void getNodeOrDefault(const std::string& path, YAML::Node& node) const
  {
    if (!getNode(path, node)) {
      auto pos = _defaults.find(path);
      bool hasDefault = pos != _defaults.end();
      OE_CHECK_FMT(hasDefault, "Attempt to read undefined config path '%s' that does not have a default value.", path.c_str());
      if (hasDefault) {
        node.reset(pos->second);
      }
    }
  }

  template<typename TValue> TValue readValue(const std::string& configPath)
  {
    YAML::Node currentElement;
    getNodeOrDefault(configPath, currentElement);
    if (currentElement.IsScalar()) {
      try {
        return currentElement.as<TValue>();
      } catch (const YAML::RepresentationException& ex) {
        LOG(FATAL) << "Failed to read config value '" << configPath << "': " << ex.what();
      }
    }
    return {};
  }

  template<typename TValue> std::vector<TValue> readValueList(const std::string& configPath)
  {
    YAML::Node currentElement;
    getNodeOrDefault(configPath, currentElement);

    checkElementType(configPath, currentElement, YAML::NodeType::Sequence);

    std::vector<TValue> values;
    for (uint32_t i = 0; i < currentElement.size(); ++i) {
      YAML::Node node = currentElement[i];
      if (!node.IsScalar()) {
        LOG(WARNING) << configPath << "[" << i << "] is not a scalar, skipping.";
        continue;
      }
      try {
        values.push_back(node.as<TValue>());
      } catch (const YAML::RepresentationException& ex) {
        LOG(FATAL) << "Failed to read config value '" << configPath << "[" << i << "]': " << ex.what();
      }
    }
    return values;
  }

  template<typename TValue> std::unordered_map<std::string, TValue> readValueDict(const std::string& configPath) const
  {
    YAML::Node currentElement;
    getNodeOrDefault(configPath, currentElement);

    checkElementType(configPath, currentElement, YAML::NodeType::Map);

    std::unordered_map<std::string, TValue> result;
    for (const auto& iter : currentElement) {
      std::string key;
      try {
        key = iter.first.as<std::string>();
      } catch (const YAML::RepresentationException& ex) {
        LOG(FATAL) << "Failed to read config value '" << configPath << "' dictionary key: " << ex.what();
      }

      try {
        result.insert({key, iter.second.as<TValue>()});
      } catch (const YAML::RepresentationException& ex) {
        LOG(FATAL) << "Failed to read config value '" << configPath << "[" << key << "]': " << ex.what();
      }
    }
    return result;
  }

 private:
  YAML::Node _root;

  std::unordered_map<std::string, YAML::Node> _defaults;
};
}
Yaml_config_reader::Yaml_config_reader(const std::wstring& configFileName)
    : _fileProcessor(new internal::Yaml_file_processor(configFileName))
{}

Yaml_config_reader::Yaml_config_reader(std::iostream& config)
    : _fileProcessor(new internal::Yaml_file_processor(config))
{}

Yaml_config_reader::~Yaml_config_reader()
{
  delete _fileProcessor;
}

void Yaml_config_reader::setDefault(const std::string& configPath, const std::string& value) const
{
  _fileProcessor->setDefault(configPath, value);
}
void Yaml_config_reader::setDefault(const std::string& configPath, int64_t value) const
{
  _fileProcessor->setDefault(configPath, value);
}

void Yaml_config_reader::setDefault(const std::string& configPath, double value) const
{
  _fileProcessor->setDefault(configPath, value);
}

void Yaml_config_reader::setDefault(const std::string& configPath, bool value) const
{
  _fileProcessor->setDefault(configPath, value);
}

void Yaml_config_reader::setDefault(const std::string& configPath, const std::vector<std::string>& value) const
{
  _fileProcessor->setDefault(configPath, value);
}

void Yaml_config_reader::setDefault(const std::string& configPath, const std::vector<int64_t>& value) const
{
  _fileProcessor->setDefault(configPath, value);
}

void Yaml_config_reader::setDefault(const std::string& configPath, const std::vector<double>& value) const
{
  _fileProcessor->setDefault(configPath, value);
}
void Yaml_config_reader::setDefault(const std::string& configPath, const std::vector<bool>& value) const
{
  _fileProcessor->setDefault(configPath, value);
}

std::string Yaml_config_reader::readString(const std::string& configPath) const
{
  return _fileProcessor->readValue<std::string>(configPath);
}

int64_t Yaml_config_reader::readInt(const std::string& configPath) const
{
  return _fileProcessor->readValue<int64_t>(configPath);
}

double Yaml_config_reader::readDouble(const std::string& configPath) const
{
  return _fileProcessor->readValue<double>(configPath);
}

bool Yaml_config_reader::readBool(const std::string& configPath) const
{
  return _fileProcessor->readValue<bool>(configPath);
}

std::vector<std::string> Yaml_config_reader::readStringList(const std::string& configPath) const
{
  return _fileProcessor->readValueList<std::string>(configPath);
}

std::vector<int64_t> Yaml_config_reader::readIntList(const std::string& configPath) const
{
  return _fileProcessor->readValueList<int64_t>(configPath);
}

std::vector<double> Yaml_config_reader::readDoubleList(const std::string& configPath) const
{
  return _fileProcessor->readValueList<double>(configPath);
}

std::vector<bool> Yaml_config_reader::readBoolList(const std::string& configPath) const
{
  return _fileProcessor->readValueList<bool>(configPath);
}

std::unordered_map<std::string, std::string> Yaml_config_reader::readStringDict(const std::string& configPath) const
{
  return _fileProcessor->readValueDict<std::string>(configPath);
}

std::unordered_map<std::string, int64_t> Yaml_config_reader::readIntDict(const std::string& configPath) const
{
  return _fileProcessor->readValueDict<int64_t>(configPath);
}

std::unordered_map<std::string, double> Yaml_config_reader::readDoubleDict(const std::string& configPath) const
{
  return _fileProcessor->readValueDict<double>(configPath);
}

std::unordered_map<std::string, bool> Yaml_config_reader::readBoolDict(const std::string& configPath) const
{
  return _fileProcessor->readValueDict<bool>(configPath);
}

size_t Yaml_config_reader::getListSize(const std::string& configPath) const
{
  YAML::Node currentElement;
  _fileProcessor->getNodeOrDefault(configPath, currentElement);
  internal::Yaml_file_processor::checkElementType(configPath, currentElement, YAML::NodeType::Sequence);

  return currentElement.size();
}
std::string Yaml_config_reader::getListElementPath(const std::string& configPath, size_t elementIdx) const
{
  return configPath + "." + std::to_string(elementIdx);
}

std::vector<std::string> Yaml_config_reader::getDictKeys(const std::string& configPath) const
{
  std::vector<std::string> result;

  YAML::Node currentElement;
  _fileProcessor->getNodeOrDefault(configPath, currentElement);
  internal::Yaml_file_processor::checkElementType(configPath, currentElement, YAML::NodeType::Map);

  for (const auto& iter : currentElement) {
    result.push_back(iter.first.as<std::string>());
  }
  return result;
}
}