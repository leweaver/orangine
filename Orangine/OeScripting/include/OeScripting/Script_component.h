#pragma once

#include "OeCore/Component.h"

#include <memory>

namespace oe {
namespace internal {
class Entity_scripting_manager;
struct Script_runtime_data;
} // namespace internal

class Script_component : public Component {
  friend ::oe::internal::Entity_scripting_manager;

  DECLARE_COMPONENT_TYPE;

 public:
  explicit Script_component(Entity& entity) : Component(entity) {}

  const std::string& scriptName() const;
  void setScriptName(const std::string& scriptName);

  // So we can have a unique_ptr to an incomplete type
  struct Script_runtime_data_deleter {
    constexpr Script_runtime_data_deleter() noexcept = default;
    void operator()(internal::Script_runtime_data* _Ptr) const noexcept;
  };

 private:
  std::string _scriptName;

  std::unique_ptr<internal::Script_runtime_data, Script_runtime_data_deleter> _scriptRuntimeData;
};
} // namespace oe
