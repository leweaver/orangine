#pragma once

#include "Manager_base.h"

#include <unordered_map>
#include <string>

namespace oe {
class IAsset_manager {
 public:
  virtual ~IAsset_manager() = default;

  // Sets the default path that contains models/textures/shaders etc.
  // Must be called before initialize(), otherwise an exception will be thrown.
  virtual void preInit_setDataPath(const std::string&) = 0;
  virtual const std::string& getDataPath() const = 0;

  virtual void setDataPathOverrides(std::unordered_map<std::string, std::string> paths) = 0;
  virtual const std::unordered_map<std::string, std::string>& dataPathOverrides(
      std::unordered_map<std::string, std::string>&& paths) const = 0;

  // If true, any calls to `makeAbsoluteAssetPath` will fail if they do not match
  // a prefix in the dataPathOverrides map. Default value: false.
  virtual void setFallbackDataPathAllowed(bool allow) = 0;
  virtual bool fallbackDataPathAllowed() const = 0;

  virtual std::string makeAbsoluteAssetPath(const std::string& path) const = 0;
};
} // namespace oe