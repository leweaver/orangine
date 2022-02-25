#pragma once

#include <OeCore/Manager_base.h>

namespace oe {
class IEntity_scripting_manager {
 public:
  virtual void renderImGui() = 0;

  // Python commands
  virtual void execute(const std::string& command) = 0;
  virtual bool commandSuggestions(const std::string& command,
                                  std::vector<std::string>& suggestions) = 0;

  // Python class name - can include a module (sych as mygame.MyClass)
  virtual void loadSceneScript(const std::string& scriptClassString) = 0;
};
} // namespace oe