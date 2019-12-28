#include "pch.h"

#include "Behavior_manager.h"
#include "OeCore/Scene.h"

using namespace oe;

template <> IBehavior_manager* oe::create_manager(Scene& scene)
{
  return new Behavior_manager(scene);
}

void Behavior_manager::tick()
{
  for (auto& efb : _entityFilterBehaviors) {
    const auto behavior = efb.behavior.get();
    // TODO: When cpp20 is here, we can call handleEntity with a range of entities, to avoid
    // repeatedly making this virtual function call!
    for (auto& entity : *efb.entityFilter) {
      behavior->handleEntity(*entity);
    }
  }
}

void Behavior_manager::addForComponentType(std::unique_ptr<Behavior_base> behavior,
                                           Component::Component_type componentType)
{
  addForComponentTypes(std::move(behavior), {componentType});
}

void Behavior_manager::addForComponentTypes(
    std::unique_ptr<Behavior_base> behavior,
    std::unordered_set<Component::Component_type> componentTypes, Entity_filter_mode mode)
{
  if (_nameToBehaviorMap.find(behavior->name()) != _nameToBehaviorMap.end()) {
    throw std::invalid_argument("Behavior with name '" + behavior->name() +
                                "' already added to Behavior_manager");
  }
  _nameToBehaviorMap[behavior->name()] = behavior.get();

  const auto filter = _scene.manager<IScene_graph_manager>().getEntityFilter(componentTypes, mode);
  _entityFilterBehaviors.push_back({filter, std::move(behavior)});
}
