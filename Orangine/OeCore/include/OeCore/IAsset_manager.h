#pragma once

#include "Manager_base.h"

#include <map>
#include <string>

namespace oe {
class IAsset_manager {
 public:
  virtual ~IAsset_manager() = default;

  // Sets the default path that contains models/textures/shaders etc.
  // Must be called before initialize(), otherwise an exception will be thrown.
  virtual void preInit_setDataPath(const std::wstring&) = 0;
  virtual const std::wstring& getDataPath() const = 0;

  virtual void setDataPathOverrides(std::map<std::wstring, std::wstring>&& paths) = 0;
  virtual const std::map<std::wstring, std::wstring>& dataPathOverrides(
      std::map<std::wstring, std::wstring>&& paths) const = 0;

  // If true, any calls to `makeAbsoluteAssetPath` will fail if they do not match
  // a prefix in the dataPathOverrides map. Default value: false.
  virtual void setFallbackDataPathAllowed(bool allow) = 0;
  virtual bool fallbackDataPathAllowed() const = 0;

  virtual std::wstring makeAbsoluteAssetPath(const std::wstring& path) const = 0;
};
} // namespace oe