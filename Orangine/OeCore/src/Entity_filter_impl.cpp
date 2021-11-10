#include "pch.h"

#include "Entity_filter_impl.h"

#include <OeCore/Entity.h>

using namespace oe;

Entity_filter_impl::Entity_filter_impl(const Component_type_set::const_iterator& begin,
                                       const Component_type_set::const_iterator& end,
                                       Entity_filter_mode mode)
    : mode(mode)
{
  componentTypes = std::unordered_set<Component::Component_type>(begin, end);
}

void Entity_filter_impl::handleEntityAdd(std::shared_ptr<Entity> entity)
{
  // Add it to the filter if all of the components we look for is present.
  const auto componentCount = entity->getComponentCount();

  if (mode == Entity_filter_mode::All) {
    // An entity can contain multiple instances of a single component type.
    Component_type_set foundComponents;
    for (size_t i = 0; i < componentCount; i++) {
      const auto& component = entity->getComponent(i);

      const auto pos = componentTypes.find(component.getType());
      if (pos == componentTypes.end())
        continue;

      foundComponents.insert(component.getType());
    }

    if (foundComponents.size() == componentTypes.size()) {
      _entities.insert(entity);
      publishEntityAdd(entity.get());
    }
  }
  else {
    assert(mode == Entity_filter_mode::Any);
    for (size_t i = 0; i < componentCount; i++) {
      const auto& component = entity->getComponent(i);

      const auto pos = componentTypes.find(component.getType());
      if (pos != componentTypes.end()) {
        _entities.insert(entity);
        publishEntityAdd(entity.get());
        return;
      }
    }
  }
}

void Entity_filter_impl::handleEntityRemove(std::shared_ptr<Entity> entity)
{
  // Remove it from the filter if any one of the components we look for is present (early exit).
  const auto componentCount = entity->getComponentCount();
  for (size_t i = 0; i < componentCount; i++) {
    const auto& component = entity->getComponent(i);

    const auto pos = componentTypes.find(component.getType());
    if (pos == componentTypes.end())
      continue;

    _entities.erase(entity);
    publishEntityRemove(entity.get());
    break;
  }
}

void Entity_filter_impl::handleEntityComponentsUpdated(std::shared_ptr<Entity> entity)
{
  // TODO: Optimize.
  handleEntityRemove(entity);
  handleEntityAdd(entity);
}

void Entity_filter_impl::add_listener(std::weak_ptr<Entity_filter_listener> callback)
{
  _listeners.push_back(callback);
}

void Entity_filter_impl::publishEntityAdd(Entity* entity)
{
  for (auto iter = _listeners.begin(); iter != _listeners.end(); ++iter) {
    auto& weakPtr = (*iter);
    std::shared_ptr<Entity_filter_listener> listener = weakPtr.lock();
    if (listener == nullptr) {
      iter = _listeners.erase(iter);
      continue;
    }

    (*listener).onAdd(entity);
  }
}

void Entity_filter_impl::publishEntityRemove(Entity* entity)
{
  for (auto iter = _listeners.begin(); iter != _listeners.end(); ++iter) {
    auto& weakPtr = (*iter);
    std::shared_ptr<Entity_filter_listener> listener = weakPtr.lock();
    if (listener == nullptr) {
      iter = _listeners.erase(iter);
      continue;
    }

    (*listener).onRemove(entity);
  }
}