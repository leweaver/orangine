﻿#pragma once

#include <OeCore/Manager_base.h>

namespace oe {
class IEntity_scripting_manager {
 public:
  // Only has an effect if called before the manager is initialized.
  // Appends the given absolute path to sys.path
  virtual void preInit_addAbsoluteScriptsPath(const std::string& path) = 0;

  virtual void renderImGui() = 0;

  // Python commands
  virtual void execute(const std::string& command) = 0;
  virtual bool commandSuggestions(const std::string& command,
                                  std::vector<std::string>& suggestions) = 0;

  // Python class name - can include a module (sych as mygame.MyClass)
  virtual void loadSceneScript(const std::string& scriptClassString) = 0;
};
} // namespace oe