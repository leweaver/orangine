#pragma once

#include "Manager_base.h"

namespace oe {
class IEntity_scripting_manager : public Manager_base, public Manager_tickable {
 public:
  explicit IEntity_scripting_manager(Scene& scene) : Manager_base(scene) {}

  virtual void renderImGui() = 0;

  // Python commands
  virtual void execute(const std::string& command) = 0;
  virtual bool commandSuggestions(const std::string& command,
                                  std::vector<std::string>& suggestions) = 0;
};
} // namespace oe