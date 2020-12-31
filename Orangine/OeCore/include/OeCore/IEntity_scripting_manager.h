#pragma once

#include "Manager_base.h"

namespace oe {
class IEntity_scripting_manager : public Manager_base, public Manager_tickable {
 public:
  explicit IEntity_scripting_manager(Scene& scene) : Manager_base(scene) {}

  // Only has an effect if called before the manager is initialized.
  // Appends the given absolute path to sys.path
  virtual void preInit_addAbsoluteScriptsPath(const std::wstring& path) = 0;

  virtual void renderImGui() = 0;

  // Python commands
  virtual void execute(const std::string& command) = 0;
  virtual bool commandSuggestions(const std::string& command,
                                  std::vector<std::string>& suggestions) = 0;

  // Python class name - can include a module (sych as mygame.MyClass)
  virtual void loadSceneScript(const std::string& scriptClassString) = 0;
};
} // namespace oe