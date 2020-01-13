#include "pch.h"

#include "Behavior_manager.h"
#include "OeCore/Scene.h"

using namespace oe;

template <> IBehavior_manager* oe::create_manager(Scene& scene) {
  return new Behavior_manager(scene);
}

void Behavior_manager::shutdown() {

  _entityFilterBehaviors.clear();
  _nameToEntityBehaviorMap.clear();

  for (const auto& entry : _initializedSceneBehaviors) {
    entry->shutdown(_scene);
  }

  _newSceneBehaviors.clear();
  _initializedSceneBehaviors.clear();
  _nameToSceneBehaviorMap.clear();
}

void Behavior_manager::tick() {

  for (auto& behavior : _newSceneBehaviors) {
    behavior->initialize(_scene);

    _nameToSceneBehaviorMap[behavior->name()] = behavior.get();
    _initializedSceneBehaviors.push_back(std::move(behavior));
  }
  _newSceneBehaviors.clear();

  for (const auto& behavior : _initializedSceneBehaviors) {
    behavior->handleScene(_scene);
  }

  for (auto& efb : _entityFilterBehaviors) {
    const auto behavior = efb.behavior.get();
    // TODO: When cpp20 is here, we can call handleEntity with a range of entities, to avoid
    // repeatedly making this virtual function call!
    for (auto& entity : *efb.entityFilter) {
      behavior->handleEntity(*entity);
    }
  }
}

void Behavior_manager::addForComponentType(
    std::unique_ptr<Entity_behavior> behavior,
    Component::Component_type componentType) {
  addForComponentTypes(std::move(behavior), {componentType});
}

void Behavior_manager::addForComponentTypes(
    std::unique_ptr<Entity_behavior> behavior,
    std::unordered_set<Component::Component_type> componentTypes,
    Entity_filter_mode mode) {
  if (_nameToEntityBehaviorMap.find(behavior->name()) != _nameToEntityBehaviorMap.end()) {
    throw std::invalid_argument(
        "Behavior with name '" + behavior->name() + "' already added to Behavior_manager");
  }
  _nameToEntityBehaviorMap[behavior->name()] = behavior.get();

  const auto filter = _scene.manager<IScene_graph_manager>().getEntityFilter(componentTypes, mode);
  _entityFilterBehaviors.push_back({filter, std::move(behavior)});
}

void Behavior_manager::addForScene(std::unique_ptr<Scene_behavior> behavior) {
  const auto& existingUninitialized = std::find_if(
      _newSceneBehaviors.begin(),
      _newSceneBehaviors.end(),
      [behavior = behavior.get()](const auto& sb) { return sb->name() == behavior->name(); });

  if (existingUninitialized != _newSceneBehaviors.end() || _nameToSceneBehaviorMap.find(behavior->name()) != _nameToSceneBehaviorMap.end()) {
    throw std::invalid_argument(
        "Behavior with name '" + behavior->name() + "' already added to Behavior_manager");
  }

  // Add to the list of behaviors that will be initialized next tick.
  _newSceneBehaviors.push_back(std::move(behavior));
}

void Behavior_manager::renderImGui() { for (const auto& sb : _initializedSceneBehaviors) {
    sb->renderImGui();
  } }
