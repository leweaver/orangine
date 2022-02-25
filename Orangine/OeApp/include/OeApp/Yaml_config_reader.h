#pragma once

#include <OeCore/IConfigReader.h>

#include <string>
#include <unordered_map>

namespace oe::app {
namespace internal {
class Yaml_file_processor;
}

class Yaml_config_reader : public IConfigReader {
 public:
  explicit Yaml_config_reader(const std::wstring& configFileName);
  explicit Yaml_config_reader(std::iostream& config);
  ~Yaml_config_reader() override;

  void setDefault(const std::string& configPath, const std::string& value) const override;
  void setDefault(const std::string& configPath, int64_t value) const override;
  void setDefault(const std::string& configPath, double value) const override;
  void setDefault(const std::string& configPath, bool value) const override;

  void setDefault(const std::string& configPath, const std::vector<std::string>& value) const override;
  void setDefault(const std::string& configPath, const std::vector<int64_t>& value) const override;
  void setDefault(const std::string& configPath, const std::vector<double>& value) const override;
  void setDefault(const std::string& configPath, const std::vector<bool>& value) const override;

  std::string readString(const std::string& configPath) const override;
  int64_t readInt(const std::string& configPath) const override;
  double readDouble(const std::string& configPath) const override;
  bool readBool(const std::string& configPath) const override;

  std::vector<std::string> readStringList(const std::string& configPath) const override;
  std::vector<int64_t> readIntList(const std::string& configPath) const override;
  std::vector<double> readDoubleList(const std::string& configPath) const override;
  std::vector<bool> readBoolList(const std::string& configPath) const override;

  std::unordered_map<std::string, std::string> readStringDict(const std::string& configPath) const override;
  std::unordered_map<std::string, int64_t> readIntDict(const std::string& configPath) const override;
  std::unordered_map<std::string, double> readDoubleDict(const std::string& configPath) const override;
  std::unordered_map<std::string, bool> readBoolDict(const std::string& configPath) const override;

  size_t getListSize(const std::string& configPath) const override;
  std::string getListElementPath(const std::string& configPath,size_t elementIdx) const override;
  std::vector<std::string> getDictKeys(const std::string& configPath) const override;

 private:
  internal::Yaml_file_processor* _fileProcessor;
};
} // namespace oe