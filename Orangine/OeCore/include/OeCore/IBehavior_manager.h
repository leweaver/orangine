#pragma once

#include "Component.h"
#include "Entity_behavior.h"
#include "Entity_filter_mode.h"
#include "Manager_base.h"
#include "Scene_behavior.h"

#include <unordered_set>

namespace oe {

/**
 * The main manager that you will use to add interactive functionality to your app. Supports two
 * types of behaviors:
 *
 * Scene_behavior: Ticked once per frame. Useful for things that aren't tied to a specific entity,
 *   such as scorekeeping and rules engines. This is how you add custom manager classes to your app.
 * Entity_behavior: Ticked once for each entity that meets a component filter (ie, has one or more
 *   specified component types. Useful for controlling properties of entity components, eg a
 *   player movement controller.
 */
class IBehavior_manager {
 public:
  virtual ~IBehavior_manager() = default;

  // Adds a new behavior that will be called once per tick, for any entity with the given component.
  virtual void addForComponentType(std::unique_ptr<Entity_behavior>, Component::Component_type) = 0;

  // Adds a new behavior that will be called once per tick, for any entity with any/all of the given
  // components.
  virtual void addForComponentTypes(
      std::unique_ptr<Entity_behavior>,
      std::unordered_set<Component::Component_type>,
      Entity_filter_mode mode = Entity_filter_mode::All) = 0;

  // Adds a behavior that will be called once per tick, not tied to a specific entity.
  virtual void addForScene(std::unique_ptr<Scene_behavior>) = 0;

  // Calls the renderImGui method on all scene behaviors
  virtual void renderImGui() = 0;
};
} // namespace oe
