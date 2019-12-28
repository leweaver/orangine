#pragma once

#include "Behavior_base.h"
#include "Component.h"
#include "Entity_filter_mode.h"
#include "Manager_base.h"

#include <unordered_set>

namespace oe {
class IBehavior_manager : public Manager_base, public Manager_tickable {
 public:
  explicit IBehavior_manager(Scene& scene) : Manager_base(scene) {}

  // Adds a new behavior that will be called once per tick, for any entity with the given component.
  virtual void addForComponentType(std::unique_ptr<Behavior_base>, Component::Component_type) = 0;

  // Adds a new behavior that will be called once per tick, for any entity with any/all of the given
  // components.
  virtual void addForComponentTypes(std::unique_ptr<Behavior_base>,
                                    std::unordered_set<Component::Component_type>,
                                    Entity_filter_mode mode = Entity_filter_mode::All) = 0;
};
} // namespace oe
