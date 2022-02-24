#pragma once

#include <string>
#include <unordered_map>
#include <gsl/span>

namespace oe {
class IConfigReader {
 public:
  virtual ~IConfigReader() = default;

  virtual void setDefault(const std::string& configPath, const std::string& value) const = 0;
  virtual void setDefault(const std::string& configPath, int64_t value) const = 0;
  virtual void setDefault(const std::string& configPath, double value) const = 0;
  virtual void setDefault(const std::string& configPath, bool value) const = 0;

  virtual void setDefault(const std::string& configPath, const std::vector<std::string>& value) const = 0;
  virtual void setDefault(const std::string& configPath, const std::vector<int64_t>& value) const = 0;
  virtual void setDefault(const std::string& configPath, const std::vector<double>& value) const = 0;
  virtual void setDefault(const std::string& configPath, const std::vector<bool>& value) const = 0;

  virtual std::string readString(const std::string& configPath) const = 0;
  virtual int64_t readInt(const std::string& configPath) const = 0;
  virtual double readDouble(const std::string& configPath) const = 0;
  virtual bool readBool(const std::string& configPath) const = 0;

  virtual std::vector<std::string> readStringList(const std::string& configPath) const = 0;
  virtual std::vector<int64_t> readIntList(const std::string& configPath) const = 0;
  virtual std::vector<double> readDoubleList(const std::string& configPath) const = 0;
  virtual std::vector<bool> readBoolList(const std::string& configPath) const = 0;

  virtual std::unordered_map<std::string, std::string> readStringDict(const std::string& configPath) const = 0;
  virtual std::unordered_map<std::string, int64_t> readIntDict(const std::string& configPath) const = 0;
  virtual std::unordered_map<std::string, double> readDoubleDict(const std::string& configPath) const = 0;
  virtual std::unordered_map<std::string, bool> readBoolDict(const std::string& configPath) const = 0;

  virtual size_t getListSize(const std::string& configPath) const = 0;
  virtual std::string getListElementPath(const std::string& configPath,size_t elementIdx) const = 0;
  virtual std::vector<std::string> getDictKeys(const std::string& configPath) const = 0;
};

}// namespace oe