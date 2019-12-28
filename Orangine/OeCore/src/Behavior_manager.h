#pragma once

#include <OeCore/IBehavior_manager.h>

namespace oe {
class Entity_filter;
class Behavior_manager : public IBehavior_manager {
 public:
  explicit Behavior_manager(Scene& scene) : IBehavior_manager(scene) {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // Manager_tickable implementation
  void tick() override;

  // IBehavior_manager implementation
  void addForComponentType(std::unique_ptr<Behavior_base>, Component::Component_type) override;
  void addForComponentTypes(std::unique_ptr<Behavior_base>,
                            std::unordered_set<Component::Component_type>,
                            Entity_filter_mode mode = Entity_filter_mode::All) override;

 private:
  const std::string _name = "Behavior_manager";

  struct Entity_filter_behavior {
    std::shared_ptr<Entity_filter> entityFilter;
    std::unique_ptr<Behavior_base> behavior;
  };
  std::vector<Entity_filter_behavior> _entityFilterBehaviors;
  std::unordered_map<std::string, Behavior_base*> _nameToBehaviorMap = {};
};
} // namespace oe